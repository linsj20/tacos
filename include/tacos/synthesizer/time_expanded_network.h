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
#include <vector>
#include <map>

namespace tacos {

class TimeExpandedNetwork {
  public:
    using Time = EventQueue::Time;
    using NpuID = Topology::NpuID;

    explicit TimeExpandedNetwork(std::shared_ptr<Topology> topology) noexcept;

    void updateCurrentTime(Time newCurrentTime) noexcept;

    std::set<std::pair<NpuID, const Path *>> backtrackTEN(NpuID dest) const noexcept;

    void markLinkOccupied(NpuID src, NpuID dest) noexcept;

    void assignPath(const Path *path) noexcept;

  private:
    Time currentTime = 0;

    int npusCount;
    std::shared_ptr<Topology> topology;

    std::vector<std::vector<std::vector<const Path *>>> linkCondition = {};
    std::map<const Path *, std::pair<double, double>> pathsInUse = {};

    void updateLinkAvailability(Time delta) noexcept;

    [[nodiscard]] bool checkLinkAvailability(NpuID src,
                                             NpuID dest) const noexcept;
};

}  // namespace tacos
