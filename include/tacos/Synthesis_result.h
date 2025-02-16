/******************************************************************************
This source code is licensed under the MIT license found in the
LICENSE file in the root directory of this source tree.

Copyright (c) 2022 Intel Corporation
Copyright (c) 2022 Georgia Institute of Technology
*******************************************************************************/

#pragma once

#include <memory>
#include "Collective.h"
#include "EventQueue.h"
#include "Topology.h"
#include "Npu_result.h"
#include <vector>

namespace Tacos {

class SynthesisResult {
  public:
    //using Time = EventQueue::Time;
    //using NpuId = Topology::NpuId;

    SynthesisResult(std::shared_ptr<Topology> topology,
                    std::shared_ptr<Collective> collective) noexcept;

    [[nodiscard]] NpuResult& npu(NpuId id) noexcept;

    void collectiveTime(Time time) noexcept;
    [[nodiscard]] Time collectiveTime() const noexcept;

  private:
    int npusCount_;
    int chunksPerNpu_;
    std::vector<NpuResult> npus_;

    Time collectiveTime_;
};

}  // namespace tacos
