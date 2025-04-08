/******************************************************************************
This source code is licensed under the MIT license found in the
LICENSE file in the root directory of this source tree.

Copyright (c) 2022 Intel Corporation
Copyright (c) 2022 Georgia Institute of Technology
*******************************************************************************/

#include <cassert>
#include <memory>
#include <tacos/synthesizer/time_expanded_network.h>
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

    linkCondition.resize(npusCount + switchCount, std::vector<std::vector<const Path*>>(npusCount + switchCount, std::vector<const Path*>()));
}

std::set<std::pair<TimeExpandedNetwork::NpuID, const Path *>> TimeExpandedNetwork::backtrackTEN(
    const NpuID dest) noexcept {
    assert(0 <= dest && dest < npusCount);

    const size_t allowedContention = 1;

    // check available source links
    //TODO (linsj20) change linkUserCount from bool to a path_ptr
    auto sourceNpus = std::set<std::pair<TimeExpandedNetwork::NpuID, const Path *>>();
    for (auto src = 0; src < npusCount; src++) {
        if (src == dest) {
            continue;
        }
        const auto& paths = topology->getPaths(src, dest);
        bool npuAvailable = false;
        const Path *chosenPath = nullptr;
        size_t bestContentionUsers = allowedContention;
        for (auto& p : *paths) {
            auto a = p.path->begin(), b = a;
            b++;
            bool pathAvailable = true;
            size_t contentionUsers = 0;
            while (b != p.path->end()) {
                if (linkCondition[*a][*b].size() >= allowedContention) {
                    pathAvailable = false;
                    break;
                }
                contentionUsers = std::max(contentionUsers, 
                                           linkCondition[*a][*b].size());
                a++; b++;
            }
            if (pathAvailable) {
                if (chosenPath == nullptr || 
                        contentionUsers < bestContentionUsers) {
                    chosenPath = &p;
                    bestContentionUsers = contentionUsers;
                }
                npuAvailable = true;
            }
        }
        if (npuAvailable) {
            sourceNpus.insert(std::pair<NpuID, const Path *>(src, chosenPath));
        }
    }

    return sourceNpus;
}

std::shared_ptr<std::vector<std::tuple<NpuID, NpuID, ChunkID>>> TimeExpandedNetwork::updateCurrentTime(
    const Time newCurrentTime) noexcept {
    // TODO 3. Finalize PathOccupied
    assert(newCurrentTime > currentTime);

    // update link availability
    auto finished = updateLinkAvailability(newCurrentTime - currentTime);

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

    currentTime = newCurrentTime;

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
    for (auto a : pathsInUse) {
        printf("%p ", a.first);
    }
    printf("\n");
    //printf("%ld\n", pathsInUse.size());
    assert (pathsInUse.find(p) == pathsInUse.end());
    pathsInUse[p] = std::tuple<ChunkID, double, double>(chunk, (double)topology->getChunkSize(), p->latency);
    auto a = p->path->begin(), b = a;
    b++;
    while (b != p->path->end()) {
        linkCondition[*a][*b].push_back(p);
        a++; b++;
    }
}

std::shared_ptr<std::vector<std::tuple<NpuID, NpuID, ChunkID>>> TimeExpandedNetwork::updateLinkAvailability(const Time delta) noexcept {
    for (auto& pair : pathsInUse) {
        auto& p = pair.first;
        auto& [chunk, workload, latency] = pair.second;
        if (latency > delta) {
            latency -= delta;
            continue;
        }
        auto a = p->path->begin(), b = a;
        b++;
        Topology::Bandwidth bd = std::numeric_limits<double>::max();
        while (b != p->path->end()) {
            printf("%d %d: %f\n", *a, *b, topology->getBandwidth(*a, *b) / 
                            linkCondition[*a][*b].size());
            bd = std::min(
                    bd, 
                    topology->getBandwidth(*a, *b) / 
                            linkCondition[*a][*b].size());
            a++; b++;
        }
        printf("%f %f\n", workload, bd);
        workload -= (delta - latency) * bd;

        latency = 0;
        printf("[%d->%d]: Chunk %d, latency left %f, workload left %f\n", p->path->front(), p->path->back(), chunk, latency, workload);
    }

    std::vector<const Path *> toRemove;
    for (auto& pair : pathsInUse) {
        const Path *p = pair.first;
        auto& [chunk, workload, latency] = pair.second;
        if (workload <= 1e-9) {
            printf("finished: [%d->%d] along path %p\n", p->path->front(), p->path->back(), p);
            auto a = p->path->begin(), b = a;
            b++;
            while (b != p->path->end()) {
                auto pos = std::find(linkCondition[*a][*b].begin(), 
                            linkCondition[*a][*b].end(), p);
                assert (pos != linkCondition[*a][*b].end());
                linkCondition[*a][*b].erase(pos);
                a++; b++;
            }
            toRemove.push_back(p);
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
