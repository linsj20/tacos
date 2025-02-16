/******************************************************************************
This source code is licensed under the MIT license found in the
LICENSE file in the root directory of this source tree.

Copyright (c) 2022 Intel Corporation
Copyright (c) 2022 Georgia Institute of Technology
*******************************************************************************/

#include <cassert>
#include "Link_result.h"
#include "Npu_result.h"

using namespace Tacos;

LinkResult::LinkResult(const LinkId linkId, const LinkType type, NpuResult* const npu) noexcept
    : id_(linkId),
      type_(type),
      npu_(*npu) {}

LinkResult::LinkId LinkResult::id() const noexcept {
    return id_;
}

void LinkResult::send(const ChunkId chunk) noexcept {
    assert(type_ == LinkType::Egress);

    // create op
    const auto opId = currentOpId();
    ops_.emplace(opId, CommOp(chunk, id(), opId));
    auto& op = ops_.at(opId);

    // check and set dependent op
    auto* const depOp = npu_.getDep(chunk);
    if (depOp == nullptr) {
        // initial chunk
        return;
    }
    op.setDepOp(depOp);
}

void LinkResult::recv(const ChunkId chunk) noexcept {
    assert(type_ == LinkType::Ingress);

    // schedule this chunk
    const auto opId = currentOpId();
    ops_.emplace(opId, CommOp(chunk, id(), opId));

    // register this to the NPU
    auto* const depOp = &ops_.at(opId);
    npu_.registerRecvDep(chunk, depOp);
}

LinkResult::OpId LinkResult::currentOpId() noexcept {
    const auto currentId = nextOpId_;
    nextOpId_++;

    return currentId;
}

const std::map<LinkResult::OpId, CommOp>& LinkResult::ops() const noexcept {
    return ops_;
}
