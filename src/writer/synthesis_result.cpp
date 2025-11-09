/******************************************************************************
This source code is licensed under the MIT license found in the
LICENSE file in the root directory of this source tree.

Copyright (c) 2022-2025 Intel Corporation
Copyright (c) 2022-2025 Georgia Institute of Technology
*******************************************************************************/

#include <cassert>
#include <tacos/writer/synthesis_result.h>

using namespace tacos;

SynthesisResult::SynthesisResult(const Topology& topology, const Collective& collective) noexcept
    : npusCount_(topology.npusCount()) {
    npus_.reserve(npusCount_);
    for (auto npu = 0; npu < npusCount_; npu++) {
        npus_.emplace_back(npu, topology);
    }
    for (auto chunk = 0; chunk < collective.chunksCount(); chunk++) {
        const auto src = collective.precondition(chunk);
        npus_[src].registerRecvDep(chunk, nullptr);
    }
}

NpuResult& SynthesisResult::npu(const NpuID id) noexcept {
    assert(0 <= id && id < npusCount_);
    return npus_[id];
}

void SynthesisResult::collectiveTime(Time time) noexcept {
    assert(time > 0);
    collectiveTime_ = time;
}

EventQueue::Time SynthesisResult::collectiveTime() const noexcept {
    assert(collectiveTime_ > 0);
    return collectiveTime_;
}
