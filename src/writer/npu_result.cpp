/******************************************************************************
This source code is licensed under the MIT license found in the
LICENSE file in the root directory of this source tree.

Copyright (c) 2022-2025 Intel Corporation
Copyright (c) 2022-2025 Georgia Institute of Technology
*******************************************************************************/

#include <cassert>
#include <tacos/writer/npu_result.h>

using namespace tacos;

NpuResult::NpuResult(const NpuID id, const Topology& topology) noexcept
    : npusCount_(topology.npusCount()), id_(id) {
    for (auto npu = 0; npu < npusCount_; npu++) {
        if (npu == id) {
            continue;
        }
        if (topology.connected(npu, id)) {
            ingressLinks_.emplace(npu, LinkResult(nextLinkId_, LinkType::Ingress, this));
            nextLinkId_++;
        }
    }
    for (auto npu = 0; npu < npusCount_; npu++) {
        if (npu == id) {
            continue;
        }
        if (topology.connected(id, npu)) {
            egressLinks_.emplace(npu, LinkResult(nextLinkId_, LinkType::Egress, this));
            nextLinkId_++;
        }
    }
}

LinkResult& NpuResult::linkFrom(const NpuID id) noexcept {
    assert(0 <= id && id < npusCount_);
    assert(ingressLinks_.find(id) != ingressLinks_.end());
    return ingressLinks_.at(id);
}

LinkResult& NpuResult::linkTo(const NpuID id) noexcept {
    assert(0 <= id && id < npusCount_);
    assert(egressLinks_.find(id) != egressLinks_.end());
    return egressLinks_.at(id);
}

void NpuResult::registerRecvDep(const ChunkID chunk, CommOp* const depOp) noexcept {
    assert(depRecvOp_.find(chunk) == depRecvOp_.end());
    depRecvOp_.emplace(chunk, depOp);
}

CommOp* NpuResult::getDep(const ChunkID chunk) noexcept {
    if (depRecvOp_.find(chunk) == depRecvOp_.end()) {
        return nullptr;
    }
    return depRecvOp_.at(chunk);
}

const std::map<NpuResult::NpuID, LinkResult>& NpuResult::ingressLinks() const noexcept {
    return ingressLinks_;
}

const std::map<NpuResult::NpuID, LinkResult>& NpuResult::egressLinks() const noexcept {
    return egressLinks_;
}
