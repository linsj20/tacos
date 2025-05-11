/******************************************************************************
This source code is licensed under the MIT license found in the
LICENSE file in the root directory of this source tree.

Copyright (c) 2022 Intel Corporation
Copyright (c) 2022 Georgia Institute of Technology
*******************************************************************************/

#pragma once

#include <cstdint>
#include <set>
#include <tacos/event-queue/event_queue.h>
#include <tuple>
#include <unordered_set>
#include <vector>

namespace tacos {

struct Path {
    using NpuID = int;
    using Latency = double;      // ns
    using Bandwidth = double;    // GB/s ~ B/ns
    std::vector<NpuID>* path;
    Bandwidth bandwidth;
    Latency latency;
};

class Topology {

  public:
    using Time = EventQueue::Time;

    using NpuID = int;
    using Latency = double;      // ns
    using Bandwidth = double;    // GB/s ~ B/ns
    using ChunkSize = uint64_t;  // B

    Topology() noexcept;

    [[nodiscard]] bool isConnected(NpuID src, NpuID dest) const noexcept;

    void setChunkSize(ChunkSize newChunkSize) noexcept;

    [[nodiscard]] Time getLinkDelay(NpuID src, NpuID dest) const noexcept;

    [[nodiscard]] int getNpusCount() const noexcept;

    [[nodiscard]] int getSwitchCount() const noexcept;

    [[nodiscard]] std::set<Time> getDistinctLinkDelays() const noexcept;

    [[nodiscard]] int getLinksCount() const noexcept;

    [[nodiscard]] Latency getLatency(NpuID src, NpuID dest) const noexcept;

    [[nodiscard]] Bandwidth getBandwidth(NpuID src, NpuID dest) const noexcept;

    [[nodiscard]] const std::vector<Path>* getPaths(NpuID src, NpuID dest) const noexcept;

    void setPath() noexcept;

    ChunkSize getChunkSize() const noexcept {
        return chunkSize;
    }

    int getGroupSize() const noexcept {
        return groupCount;
    }

    int getGroup(NpuID npu) const noexcept;

  protected:
    void setNpusCount(int newNpusCount, int newSwitchSize=0) noexcept;

    void connect(NpuID src,
                 NpuID dest,
                 Latency latency,
                 Bandwidth bandwidth,
                 bool bidirectional = false) noexcept;

  private:
    int npusCount = -1;
    int switchCount = -1;
    int linksCount = 0;
    int groupCount = 0;
    bool npusCountSet = false;
    bool pathSet = false;

    std::set<Time> distinctLinkDelays = {};

    std::vector<std::vector<bool>> connected = {};
    std::vector<std::vector<Latency>> latencies = {};
    std::vector<std::vector<Bandwidth>> bandwidths = {};
    //std::vector<std::vector<std::set<Time>>> linkDelays = {};
    std::vector<std::vector<Time>> linkDelays = {};
    std::vector<std::vector<std::vector<Path>>> shortestPaths = {};
    std::vector<std::vector<int>> groupInfo = {};
    std::vector<std::unordered_set<NpuID>> groups = {};

    Time computeLinkDelay(NpuID src, NpuID dest) const noexcept;

    void setGroup(std::vector<std::vector<Latency>> dists) noexcept;

    int allocateGroup() noexcept;

    ChunkSize chunkSize = -1;
    bool chunkSizeSet = false;
};

}  // namespace tacos
