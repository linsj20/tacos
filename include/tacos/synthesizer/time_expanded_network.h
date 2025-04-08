/******************************************************************************
This source code is licensed under the MIT license found in the
LICENSE file in the root directory of this source tree.

Copyright (c) 2022 Intel Corporation
Copyright (c) 2022 Georgia Institute of Technology
*******************************************************************************/

#pragma once

#include <memory>
#include <tacos/event-queue/event_queue.h>
#include <tacos/topology/topology.h>
#include <tacos/collective/collective.h>
#include <vector>
#include <map>
#include <tuple>

namespace tacos {

class TimeExpandedNetwork {
  public:
    using Time = EventQueue::Time;
    using NpuID = Topology::NpuID;
    using ChunkID = Collective::ChunkID;

    explicit TimeExpandedNetwork(std::shared_ptr<Topology> topology) noexcept;

    std::shared_ptr<std::vector<std::tuple<NpuID, NpuID, ChunkID>>> updateCurrentTime(Time newCurrentTime) noexcept;

    std::set<std::pair<NpuID, const Path *>> backtrackTEN(NpuID dest) noexcept;

    void markLinkOccupied(NpuID src, NpuID dest) noexcept;

    void assignPath(const Path *path, ChunkID chunk) noexcept;

    bool complete() const noexcept;

  private:
    Time currentTime = 0;

    int npusCount, switchCount;
    std::shared_ptr<Topology> topology;

    std::vector<std::vector<std::vector<const Path *>>> linkCondition = {};
    std::map<const Path *, std::tuple<ChunkID, double, double>> pathsInUse = {};

    std::shared_ptr<std::vector<std::tuple<NpuID, NpuID, ChunkID>>> updateLinkAvailability(Time delta) noexcept;
};

}  // namespace tacos
