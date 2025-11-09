/******************************************************************************
This source code is licensed under the MIT license found in the
LICENSE file in the root directory of this source tree.

Copyright (c) 2022-2025 Intel Corporation
Copyright (c) 2022-2025 Georgia Institute of Technology
*******************************************************************************/

#include <cassert>
#include <tacos/writer/link_result.h>
#include <tacos/writer/npu_result.h>

using namespace tacos;

LinkResult::LinkResult(const LinkID linkId, const LinkType type, NpuResult* const npu) noexcept
    : id_(linkId), type_(type), npu_(*npu) {}

LinkResult::LinkID LinkResult::id() const noexcept {
    return id_;
}

void LinkResult::send(const ChunkID chunk) noexcept {
    assert(type_ == LinkType::Egress);
    const auto opId = currentOpId();
    ops_.emplace(opId, CommOp(chunk, id(), opId));
    auto& op = ops_.at(opId);
    auto* const depOp = npu_.getDep(chunk);
    if (depOp != nullptr) {
        op.setDepOp(depOp);
    }
}

void LinkResult::recv(const ChunkID chunk) noexcept {
    assert(type_ == LinkType::Ingress);
    const auto opId = currentOpId();
    ops_.emplace(opId, CommOp(chunk, id(), opId));
    auto* const depOp = &ops_.at(opId);
    npu_.registerRecvDep(chunk, depOp);
}

LinkResult::OpID LinkResult::currentOpId() noexcept {
    const auto currentId = nextOpId_;
    nextOpId_++;
    return currentId;
}

const std::map<LinkResult::OpID, CommOp>& LinkResult::ops() const noexcept {
    return ops_;
}
