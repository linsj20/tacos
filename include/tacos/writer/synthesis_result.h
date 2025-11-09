/******************************************************************************
This source code is licensed under the MIT license found in the
LICENSE file in the root directory of this source tree.

Copyright (c) 2022-2025 Intel Corporation
Copyright (c) 2022-2025 Georgia Institute of Technology
*******************************************************************************/

#pragma once

#include <vector>
#include <tacos/collective/collective.h>
#include <tacos/event_queue/event_queue.h>
#include <tacos/topology/topology.h>
#include <tacos/writer/npu_result.h>

namespace tacos {

class SynthesisResult {
  public:
    using Time = EventQueue::Time;
    using NpuID = Topology::NpuID;

    SynthesisResult(const Topology& topology, const Collective& collective) noexcept;

    [[nodiscard]] NpuResult& npu(NpuID id) noexcept;
    void collectiveTime(Time time) noexcept;
    [[nodiscard]] Time collectiveTime() const noexcept;

  private:
    int npusCount_;
    std::vector<NpuResult> npus_;
    Time collectiveTime_;
};

}  // namespace tacos
