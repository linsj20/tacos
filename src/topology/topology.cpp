/******************************************************************************
This source code is licensed under the MIT license found in the
LICENSE file in the root directory of this source tree.

Copyright (c) 2022 Intel Corporation
Copyright (c) 2022 Georgia Institute of Technology
*******************************************************************************/

#include <cassert>
#include <endian.h>
#include <iterator>
#include <limits>
#include <tacos/topology/topology.h>
#include <unordered_set>
#include <vector>
#include <queue>
#include <algorithm>
#include <numeric>
#include <cstdio>
#include <map>

using namespace tacos;

Topology::Topology() noexcept = default;

// TODO 0. Add switch count
void Topology::setNpusCount(const int newNpusCount, const int newSwitchesCount) noexcept {
    assert(!npusCountSet);
    assert(newSwitchesCount >= 0);
    assert(newNpusCount > 0);

    //npusCount = newNpusCount + newSwitchesCount;
    npusCount = newNpusCount;
    switchCount = newSwitchesCount;
    npusCountSet = true;

    auto deviceCount = npusCount + switchCount;

    // allocate memory
    connected.resize(deviceCount, std::vector(deviceCount, false));
    latencies.resize(deviceCount, std::vector<Latency>(deviceCount, -1));
    bandwidths.resize(deviceCount, std::vector<Bandwidth>(deviceCount, -1));
    linkDelays.resize(
        deviceCount,
        std::vector<Time>(deviceCount, std::numeric_limits<uint64_t>::max()));
    shortestPaths.resize(
        deviceCount, 
        std::vector<std::vector<Path>>(deviceCount, std::vector<Path>()));
    groupInfo.resize(deviceCount, std::vector<int>());
}

// TODO 1. Add Path (linked list)
void Topology::setPath() noexcept {
    assert(!pathSet);
    pathSet = true;
    auto deviceCount = npusCount + switchCount;

    const Latency LARGE_TIME = 1e9;
    std::vector<std::vector<Latency>> times(deviceCount, std::vector<Latency>(deviceCount, LARGE_TIME));

    // Initialize latency matrix
    for (int src = 0; src < deviceCount; ++src) {
        for (int dest = 0; dest < deviceCount; ++dest) {
            if (bandwidths[src][dest] > 0) {
                times[src][dest] = 1e6 / bandwidths[src][dest];
            }
        }
    }

    // Process each source node
    std::vector<std::vector<Latency>> dists;
    for (int src = 0; src < deviceCount; ++src) {
        // Dijkstra's algorithm for shortest distances
        std::vector<Latency> dist(deviceCount, LARGE_TIME);
        std::priority_queue<std::pair<Latency, int>,
                          std::vector<std::pair<Latency, int>>,
                          std::greater<>> pq;
        dist[src] = 0;
        pq.emplace(0, src);

        while (!pq.empty()) {
            auto [current_dist, u] = pq.top();
            pq.pop();
            if (current_dist > dist[u]) continue;

            for (int v = 0; v < deviceCount; ++v) {
                const Latency travel_time = times[u][v];
                if (travel_time == LARGE_TIME) continue;

                if (dist[v] > dist[u] + travel_time) {
                    dist[v] = dist[u] + travel_time;
                    pq.emplace(dist[v], v);
                }
            }
        }

        // Find predecessors for path reconstruction
        std::vector<std::vector<int>> predecessors(deviceCount);
        for (int u = 0; u < deviceCount; ++u) {
            if (dist[u] == LARGE_TIME) continue;
            for (int v = 0; v < deviceCount; ++v) {
                if (times[v][u] != LARGE_TIME &&
                    std::abs(dist[v] + times[v][u] - dist[u]) < 1e-9) {
                    predecessors[u].push_back(v);
                }
            }
        }

        // Reconstruct paths using dynamic programming
        std::vector<int> processing_order(deviceCount);
        std::iota(processing_order.begin(), processing_order.end(), 0);
        std::sort(processing_order.begin(), processing_order.end(),
            [&dist](int a, int b) { return dist[a] < dist[b]; });

        std::vector<std::vector<std::vector<int>>> all_paths(deviceCount);
        if (dist[src] < LARGE_TIME) {
            all_paths[src].push_back({src});
        }

        for (int u : processing_order) {
            if (u == src || dist[u] == LARGE_TIME) continue;

            for (int pred : predecessors[u]) {
                for (const auto& path : all_paths[pred]) {
                    std::vector<int> new_path = path;
                    new_path.push_back(u);
                    all_paths[u].push_back(std::move(new_path));
                }
            }
        }

        // Store results in shortestPaths
        for (int dest = 0; dest < deviceCount; ++dest) {
            auto& dest_paths = shortestPaths[src][dest];
            for (const auto& node_path : all_paths[dest]) {
                Path p;
                p.path = new std::vector<Path::NpuID>(node_path.begin(), node_path.end());
                //p.users_count = nullptr;  // Explicitly ignore as per problem statement
                dest_paths.push_back(p);
            }
        }

        dists.push_back(dist);
    }
    for (int src = 0; src < deviceCount; src++) {
        for (int dest = 0; dest < deviceCount; dest++) {
            printf("%ld", shortestPaths[src][dest].size());
            printf("(");
            for (auto npu : *shortestPaths[src][dest][0].path) {
                printf("%d->", npu);
            }
            printf(")\t");
        }
        printf("\n");
    }
    setGroup(dists);
}

int Topology::allocateGroup() noexcept {
    groups.push_back(std::unordered_set<NpuID>());
    return groupCount++;
}

void Topology::setGroup(std::vector<std::vector<Latency>> dists) noexcept {
    auto deviceCount = npusCount + switchCount;

    // Each device as a group
    std::unordered_set<int> to_group;
    for (NpuID d = 0; d < deviceCount; d++) {
        int newGroupId = allocateGroup();
        groups[newGroupId].insert(d);
        groupInfo[d].push_back(newGroupId);
        to_group.insert(newGroupId);
    }

    while (true) {
        bool newGroupCreated = false;

        std::unordered_set<int> grouped;
        for (auto& a : to_group) {
            if (grouped.find(a) != grouped.end()) continue;

            int newGroupId = -1;
            for (auto& b : to_group) {
                if (a == b || grouped.find(b) != grouped.end()) continue;

                bool match = true;
                for (auto c : to_group) {
                    if (a == c || b == c) continue;
                    NpuID da = *groups[a].begin();
                    NpuID db = *groups[b].begin();
                    NpuID dc = *groups[c].begin();
                    bool npuMatching = da < npusCount && db < npusCount;
                    bool switchMatching = da >= npusCount && db >= npusCount;
                    if (!npuMatching && !switchMatching) {
                        match = false;
                        break;
                    }
                    if (npuMatching && dc >= npusCount) continue;
                    if (switchMatching && dc < npusCount) continue;

                    if (dists[da][dc] != dists[db][dc] || 
                        dists[dc][da] != dists[dc][db]) {
                        match = false;
                        break;
                    }
                }
                if (match) {
                    if (newGroupId < 0) {
                        newGroupId = allocateGroup();
                        newGroupCreated = true;
                        grouped.insert(a);
                        for (auto& da : groups[a]) {
                            groups[newGroupId].insert(da);
                            groupInfo[da].push_back(newGroupId);
                        }
                    }
                    grouped.insert(b);
                    for (auto& db : groups[b]) {
                        groups[newGroupId].insert(db);
                        groupInfo[db].push_back(newGroupId);
                    }
                }
            }
        }
        to_group.clear();
        for (int d = 0; d < deviceCount; d++) {
            to_group.insert(groupInfo[d].back());
        }
        if (!newGroupCreated) break;
    }

    printf("\t");
    for (int src = 0; src < deviceCount; src++) {
        printf("%d\t", src);
    }
    printf("\n");
    for (int src = 0; src < deviceCount; src++) {
        printf("%d", src);
        for (int dest = 0; dest < deviceCount; dest++) {
            printf("\t%.0f", dists[src][dest]);
        }
        printf("\n");
    }
    for (int npu = 0; npu < deviceCount; npu++) {
        printf("[%d] group:", npu);
        for (auto& g : groupInfo[npu]) {
            printf(" %d", g);
        }
        printf("\n");
    }
}

void Topology::connect(const NpuID src,
                       const NpuID dest,
                       const Latency latency,
                       const Bandwidth bandwidth,
                       const bool bidirectional) noexcept {
    assert(0 <= src && src < npusCount + switchCount);
    assert(0 <= dest && dest < npusCount + switchCount);
    assert(src != dest);
    assert(latency >= 0);
    assert(bandwidth > 0);
    assert(!connected[src][dest]);

    // connect src -> dest
    connected[src][dest] = true;
    latencies[src][dest] = latency;
    bandwidths[src][dest] = bandwidth;
    linksCount++;

    // if bidirectional, connect dest -> src
    if (bidirectional) {
        connect(dest, src, latency, bandwidth, false);
    }
}

bool Topology::isConnected(const NpuID src, const NpuID dest) const noexcept {
    assert(npusCountSet);
    assert(0 <= src && src < npusCount);
    assert(0 <= dest && dest < npusCount);

    return shortestPaths[src][dest].size() > 0;
}

void Topology::setChunkSize(const ChunkSize newChunkSize) noexcept {
    assert(!chunkSizeSet);
    assert(newChunkSize > 0);

    // set chunk size
    chunkSize = newChunkSize;
    chunkSizeSet = true;

    // calculate link delays
    // TODO 2. (linsj20) consider switch bandwidth share
    // (mark link with path nums)
    for (auto src = 0; src < npusCount; src++) {
        for (auto dest = 0; dest < npusCount; dest++) {
            if (src == dest || shortestPaths[src][dest].size() == 0) {
                continue;
            }

            for (auto& p : shortestPaths[src][dest]) {
                auto& path = *p.path;
                auto delay = 0;
                Bandwidth bd = std::numeric_limits<uint64_t>::max();
                for (int cur = 0; cur < path.size() - 1; cur++) {
                    bd = std::min(bd, getBandwidth(path[cur], path[cur + 1]));
                    delay += getLatency(path[cur], path[cur + 1]);
                }
                p.bandwidth = bd;
                p.latency = delay;
                delay += chunkSize / bd;
                distinctLinkDelays.insert(delay);
            }
        }
    }
}

std::set<Topology::Time> Topology::getDistinctLinkDelays() const noexcept {
    assert(chunkSizeSet);

    return distinctLinkDelays;
}

Topology::Time Topology::computeLinkDelay(const NpuID src,
                                          const NpuID dest) const noexcept {
    assert(npusCountSet);
    assert(chunkSizeSet);

    assert(0 <= src && src < npusCount);
    assert(0 <= dest && dest < npusCount);

    // calculate beta (ns/B)
    const auto bandwidthBytesPerNS = bandwidths[src][dest] * (1 << 30) / 1e9;
    const auto beta = 1 / bandwidthBytesPerNS;

    // calculate link delay using alpha-beta model
    const auto linkDelayNS = latencies[src][dest] + (beta * chunkSize);

    // convert linkDelay to ps
    const auto linkDelayPS = linkDelayNS * 1e3;

    // return linkDelayPS (in Time format)
    return static_cast<Time>(linkDelayPS);
}

int Topology::getNpusCount() const noexcept {
    assert(npusCountSet);

    return npusCount;
}

int Topology::getSwitchCount() const noexcept {
    assert(npusCountSet);

    return switchCount;
}

Topology::Time Topology::getLinkDelay(NpuID src, NpuID dest) const noexcept {
    assert(npusCountSet);
    assert(chunkSizeSet);
    assert(0 <= src && src < npusCount);
    assert(0 <= dest && dest < npusCount);

    return linkDelays[src][dest];
}

int Topology::getLinksCount() const noexcept {
    return linksCount;
}

Topology::Latency Topology::getLatency(NpuID src, NpuID dest) const noexcept {
    assert(npusCountSet);
    assert(0 <= src && src < npusCount + switchCount);
    assert(0 <= dest && dest < npusCount + switchCount);

    return latencies[src][dest];
}

Topology::Bandwidth Topology::getBandwidth(NpuID src,
                                           NpuID dest) const noexcept {
    assert(npusCountSet);
    assert(0 <= src && src < npusCount + switchCount);
    assert(0 <= dest && dest < npusCount + switchCount);

    return bandwidths[src][dest];
}

// TODO 1. Get Path
const std::vector<Path>* Topology::getPaths(NpuID src, NpuID dest) const noexcept {
    assert(pathSet);
    assert(0 <= src && src < npusCount);
    assert(0 <= dest && dest < npusCount);

    return &shortestPaths[src][dest];
}

int Topology::getGroup(NpuID npu) const noexcept {
    auto& groups = groupInfo[npu];
    if (groups.size() <= 1) return groups[0];
    return groups[1];
}
