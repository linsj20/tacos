/******************************************************************************
This source code is licensed under the MIT license found in the
LICENSE file in the root directory of this source tree.

Copyright (c) 2022 Intel Corporation
Copyright (c) 2022 Georgia Institute of Technology
*******************************************************************************/

#include <cassert>
#include "Comm_op.h"

using namespace Tacos;

CommOp::CommOp(const ChunkId chunkId,
               const LinkId linkId,
               const OpId opId) noexcept
    : chunkId_(chunkId),
      linkId_(linkId),
      opId_(opId) {}

void CommOp::setDepOp(CommOp* const depOp) {
    hasDep_ = true;
    depOp_ = depOp;
}

//CommOp::ChunkId CommOp::chunkId() const noexcept {
ChunkId CommOp::chunkId() const noexcept {
    return chunkId_;
}

bool CommOp::hasDep() const noexcept {
    return hasDep_;
}

const CommOp* CommOp::depOp() const noexcept {
    return depOp_;
}

CommOp::LinkId CommOp::linkId() const noexcept {
    return linkId_;
}

CommOp::OpId CommOp::opId() const noexcept {
    return opId_;
}
