/******************************************************************************
This source code is licensed under the MIT license found in the
LICENSE file in the root directory of this source tree.

Copyright (c) 2022 Intel Corporation
Copyright (c) 2022 Georgia Institute of Technology
*******************************************************************************/

#include <cassert>
#include <tacos/synthesizer/time_expanded_network.h>
#include <vector>
#include <limits>

using namespace tacos;

TimeExpandedNetwork::TimeExpandedNetwork(
    const std::shared_ptr<Topology> topology) noexcept
    : topology(topology) {
    assert(topology != nullptr);

    npusCount = topology->getNpusCount();

    linkCondition.resize(npusCount, std::vector<std::vector<const Path*>>(npusCount, std::vector<const Path*>()));
}

std::set<std::pair<TimeExpandedNetwork::NpuID, const Path *>> TimeExpandedNetwork::backtrackTEN(
    const NpuID dest) const noexcept {
    assert(0 <= dest && dest < npusCount);

    const size_t allowedContention = 2;

    // check available source links
    //TODO (linsj20) change linkUserCount from bool to a path_ptr
    auto sourceNpus = std::set<std::pair<TimeExpandedNetwork::NpuID, const Path *>>();
    for (auto src = 0; src < npusCount; src++) {
        const auto& paths = topology->getPaths(src, dest);
        bool npuAvailable = false;
        const Path *chosenPath = nullptr;
        size_t bestContentionUsers = allowedContention;
        for (auto& p : paths) {
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

void TimeExpandedNetwork::updateCurrentTime(
    const Time newCurrentTime) noexcept {
    // TODO 3. Finalize PathOccupied
    assert(newCurrentTime > currentTime);

    // update link availability
    updateLinkAvailability(newCurrentTime - currentTime);

    currentTime = newCurrentTime;
}

void TimeExpandedNetwork::markLinkOccupied(const NpuID src,
                                           const NpuID dest) noexcept {
    assert(0 <= src && src < npusCount);
    assert(0 <= dest && dest < npusCount);
    assert(linkAvailable[src][dest]);
    assert(src != dest);

    // mark the link as occupied
    const auto linkDelay = topology->getLinkDelay(src, dest);
    //linkBusyUntil[src][dest] = currentTime;
    //linkUserCount[src][dest]++;
}

// TODO 3. markPathOccupied
void TimeExpandedNetwork::assignPath(const Path *p) noexcept {
    assert (pathsInUse.find(p) == pathsInUse.end());
    pathsInUse[p] = std::pair<double, double>((double)topology->getChunkSize(), p->latency);
    auto a = p->path->begin(), b = a;
    while (b != p->path->end()) {
        linkCondition[*a][*b].push_back(p);
        a++; b++;
    }
}

void TimeExpandedNetwork::updateLinkAvailability(const Time delta) noexcept {
    for (auto& pair : pathsInUse) {
        auto& p = pair.first;
        auto& [workload, latency] = pair.second;
        if (latency > delta) {
            latency -= delta;
            continue;
        }
        auto a = p->path->begin(), b = a;
        Topology::Bandwidth bd = std::numeric_limits<uint64_t>::max();
        while (b != p->path->end()) {
            bd = std::min(
                    bd, 
                    topology->getBandwidth(*a, *b) / 
                            linkCondition[*a][*b].size());
            a++; b++;
        }
        workload -= (delta - latency) * p->bandwidth / bd;

        latency = 0;
    }

    std::vector<const Path *> toRemove;
    for (auto& pair : pathsInUse) {
        const Path *p = pair.first;
        auto& [workload, latency] = pair.second;
        if (workload <= 1e-9) {
            auto a = p->path->begin(), b = a;
            while (b != p->path->end()) {
                std::remove(linkCondition[*a][*b].begin(), 
                            linkCondition[*a][*b].end(), p);
                a++; b++;
            }
            toRemove.push_back(p);
        }
    }
    for (const auto p : toRemove) {
        pathsInUse.erase(p);
    }
}

bool TimeExpandedNetwork::checkLinkAvailability(
    const NpuID src, const NpuID dest) const noexcept {
    assert(0 <= src && src < npusCount);
    assert(0 <= dest && dest < npusCount);
    assert(src != dest);

    // the link is available only if:
    // the start time of the current transmission does not overlap
    // with the end time of the previous transmission
    const auto linkDelay = topology->getLinkDelay(src, dest);
    const auto transmissionStartTime = currentTime - linkDelay;

    // transmission start time must be valid
    if (transmissionStartTime < 0) {
        return false;
    }

    return true;
    //return transmissionStartTime >= linkBusyUntil[src][dest];
}
