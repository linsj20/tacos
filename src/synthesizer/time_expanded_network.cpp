/******************************************************************************
This source code is licensed under the MIT license found in the
LICENSE file in the root directory of this source tree.

Copyright (c) 2022 Intel Corporation
Copyright (c) 2022 Georgia Institute of Technology
*******************************************************************************/

#include <cassert>
#include <memory>
#include <tacos/synthesizer/time_expanded_network.h>
#include <unordered_set>
#include <utility>
#include <vector>
#include <limits>
#include <cstdio>

using namespace tacos;
using Time = EventQueue::Time;
using NpuID = Topology::NpuID;
using ChunkID = Collective::ChunkID;

TimeExpandedNetwork::TimeExpandedNetwork(
    const std::shared_ptr<Topology> topology) noexcept
    : topology(topology) {
    assert(topology != nullptr);

    npusCount = topology->getNpusCount();
    switchCount = topology->getSwitchCount();

    linkCondition.resize(npusCount + switchCount, std::vector<std::unordered_set<const Path*>>(npusCount + switchCount, std::unordered_set<const Path*>()));
    groupCondition.resize(topology->getGroupSize(), std::unordered_set<ChunkID>());
}

std::set<std::pair<const Path *, Topology::Bandwidth>> TimeExpandedNetwork::backtrackTEN(
    const NpuID dest, const ChunkID chunk) noexcept {
    assert(0 <= dest && dest < npusCount);

    const size_t allowedContention = 8;

    // check available source links
    //TODO (linsj20) change linkUserCount from bool to a path_ptr
    auto sourceNpus = std::set<std::pair<const Path *, Bandwidth>>();
    int target = -1;
    auto destGroup = topology->getGroup(dest);
    auto key = std::make_pair(destGroup, chunk); 
    if (groupChunks.find(key) != groupChunks.end()) {
        target = groupChunks[key];
    }
    for (auto src = 0; src < npusCount; src++) {
        auto srcGroup = topology->getGroup(src);
        auto gc = &groupCondition[destGroup];
        if (src == dest) continue;
        if (srcGroup != destGroup && gc->find(chunk) != gc->end()) continue;
        if (srcGroup == destGroup && target >= 0 && src != target) continue;
        const auto& paths = topology->getPaths(src, dest);
        bool npuAvailable = false;
        const Path *chosenPath = nullptr;
        Topology::Bandwidth bestBandwidth = std::numeric_limits<double>::min();
        for (auto& p : *paths) {
            if (pathsInUse.find(&p) != pathsInUse.end()) continue;
            auto a = p.path->begin(), b = a;
            b++;
            bool pathAvailable = true;
            Topology::Bandwidth bd = std::numeric_limits<double>::max();
            while (b != p.path->end()) {
                if (linkCondition[*a][*b].size() >= allowedContention) {
                    pathAvailable = false;
                    break;
                }
                bd = std::min(
                        bd, 
                        topology->getBandwidth(*a, *b) / 
                                linkCondition[*a][*b].size());
                a++; b++;
            }
            if (pathAvailable) {
                if (chosenPath == nullptr || 
                        bd > bestBandwidth) {
                    chosenPath = &p;
                    bestBandwidth = bd;
                }
                npuAvailable = true;
            }
        }
        if (npuAvailable) {
            sourceNpus.insert(std::pair<const Path *, Bandwidth>(chosenPath, bestBandwidth));
        }
    }

    return sourceNpus;
}

std::shared_ptr<std::vector<std::tuple<NpuID, NpuID, ChunkID>>> TimeExpandedNetwork::updateCurrentTime(
    const Time newCurrentTime) noexcept {
    // TODO 3. Finalize PathOccupied
    assert(newCurrentTime > currentTime);

    auto delta = newCurrentTime - currentTime;

    currentTime = newCurrentTime;

    // update link availability
    auto finished = updateLinkAvailability(delta);

    for (NpuID src = 0; src < npusCount + switchCount; src++) {
        for (NpuID dest = 0; dest < npusCount + switchCount; dest++) {
            printf("[%d->%d]:", src, dest);
            for (auto& p : linkCondition[src][dest]) {
                printf("%p ", p);
            }
            printf(" | ");
        }
        printf("\n");
    }

    return finished;
}

void TimeExpandedNetwork::markLinkOccupied(const NpuID src,
                                           const NpuID dest) noexcept {
    assert(0 <= src && src < npusCount);
    assert(0 <= dest && dest < npusCount);
    //assert(linkAvailable[src][dest]);
    assert(src != dest);

    // mark the link as occupied
    const auto linkDelay = topology->getLinkDelay(src, dest);
    //linkBusyUntil[src][dest] = currentTime;
    //linkUserCount[src][dest]++;
}

// TODO 3. markPathOccupied
void TimeExpandedNetwork::assignPath(const Path *p, ChunkID chunk) noexcept {
    assert (pathsInUse.find(p) == pathsInUse.end());
    pathsInUse[p] = std::tuple<ChunkID, double, double>(chunk, (double)topology->getChunkSize(), p->latency);
    auto a = p->path->begin(), b = a;
    b++;
    Topology::Bandwidth bd = std::numeric_limits<double>::max();
    while (b != p->path->end()) {
        linkCondition[*a][*b].insert(p);
        bd = std::min(
                bd, 
                topology->getBandwidth(*a, *b) / 
                        linkCondition[*a][*b].size());
        a++; b++;
    }

    auto srcGroup = topology->getGroup(p->path->front());
    auto destGroup = topology->getGroup(p->path->back());
    auto gc = &groupCondition[destGroup];
    assert (srcGroup == destGroup || gc->find(chunk) == gc->end());
    if (srcGroup == destGroup) {
        groupChunks[std::make_pair(destGroup, chunk)] = p->path->front();
    } else {
        groupChunks[std::make_pair(destGroup, chunk)] = p->path->back();
    }
    gc->insert(chunk);

    Time estimatedTime = currentTime + p->latency + (double)topology->getChunkSize() / bd;
    pathEventTime.insert(estimatedTime + 1);
    printf("Time %ld inserted\n", estimatedTime + 1);
}

std::shared_ptr<std::vector<std::tuple<NpuID, NpuID, ChunkID>>> TimeExpandedNetwork::updateLinkAvailability(const Time delta) noexcept {
    for (auto& pair : pathsInUse) {
        auto& p = pair.first;
        auto& [chunk, workload, latency] = pair.second;
        if (latency > delta) {
            latency -= delta;
            continue;
        }
        Topology::Bandwidth bd = getPathBandwidth(p);
        workload -= (delta - latency) * bd;

        latency = 0;
        printf("[%d->%d]: Chunk %d, latency left %f, workload left %f\n", p->path->front(), p->path->back(), chunk, latency, workload);
    }

    std::set<const Path *> toRemove;
    for (auto& pair : pathsInUse) {
        const Path *p = pair.first;
        auto& [chunk, workload, latency] = pair.second;
        if (workload <= 1e-9) {
            printf("finished: [%d->%d] along path %p\n", p->path->front(), p->path->back(), p);
            auto a = p->path->begin(), b = a;
            b++;
            while (b != p->path->end()) {
                auto pos = linkCondition[*a][*b].find(p);
                assert (pos != linkCondition[*a][*b].end());
                linkCondition[*a][*b].erase(pos);

                auto iter = linkCondition[*a][*b].begin();
                while (iter != linkCondition[*a][*b].end()) {
                    const Path *pathAffected = *iter;
                    auto pathBandwidth = getPathBandwidth(pathAffected);
                    auto& [_chunk, _workload, _latency] = pathsInUse[pathAffected];
                    if (_workload > 1e-5 || _latency > 1e-5) {
                        pathEventTime.insert(currentTime + _latency + _workload / pathBandwidth + 1);
                        printf("[%ld] Time %ld inserted \n", currentTime, 
                                (Time)(currentTime + _latency + _workload / pathBandwidth + 1));
                    }
                    iter++;
                }
                a++; b++;
            }
            toRemove.insert(p);
        }
    }

    auto finished = std::make_shared<std::vector<std::tuple<NpuID, NpuID, ChunkID>>>();
    for (const auto p : toRemove) {
        finished->push_back(std::tuple<NpuID, NpuID, ChunkID>(
                            p->path->front(),
                            p->path->back(),
                            std::get<0>(pathsInUse[p])));
        pathsInUse.erase(p);
    }
    return finished;
}

bool TimeExpandedNetwork::complete() const noexcept {
    return pathsInUse.size() == 0;
}

Time TimeExpandedNetwork::nextTime() noexcept {
    assert (pathEventTime.size() > 0);

    auto firstTime = pathEventTime.begin();
    Time result = *firstTime;
    assert (result > 1e-9);
    pathEventTime.erase(firstTime);
    printf("Time %ld removed\n", result);

    return result;
}

Topology::Bandwidth TimeExpandedNetwork::getPathBandwidth(const Path *p) const noexcept {
    Topology::Bandwidth bd = std::numeric_limits<double>::max();
    auto a = p->path->begin(), b = a;
    b++;
    while (b != p->path->end()) {
        bd = std::min(
                bd, 
                topology->getBandwidth(*a, *b) / 
                        linkCondition[*a][*b].size());
        a++; b++;
    }
    return bd;
}
