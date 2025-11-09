/******************************************************************************
This source code is licensed under the MIT license found in the
LICENSE file in the root directory of this source tree.

Copyright (c) 2022-2025 Intel Corporation
Copyright (c) 2022-2025 Georgia Institute of Technology
*******************************************************************************/

#include <tacos/writer/comm_op.h>

using namespace tacos;

CommOp::CommOp(const ChunkID chunkId, const LinkID linkId, const OpID opId) noexcept
    : chunkId_(chunkId), linkId_(linkId), opId_(opId) {}

void CommOp::setDepOp(CommOp* const depOp) {
    hasDep_ = true;
    depOp_ = depOp;
    depOp->setDepended();
}

void CommOp::setDepended() {
    depended_ = true;
}

bool CommOp::depended() const noexcept {
    return depended_;
}

Collective::ChunkID CommOp::chunkId() const noexcept {
    return chunkId_;
}

bool CommOp::hasDep() const noexcept {
    return hasDep_;
}

const CommOp* CommOp::depOp() const noexcept {
    return depOp_;
}

CommOp::LinkID CommOp::linkId() const noexcept {
    return linkId_;
}

CommOp::OpID CommOp::opId() const noexcept {
    return opId_;
}
