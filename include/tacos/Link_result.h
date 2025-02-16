/******************************************************************************
This source code is licensed under the MIT license found in the
LICENSE file in the root directory of this source tree.

Copyright (c) 2022 Intel Corporation
Copyright (c) 2022 Georgia Institute of Technology
*******************************************************************************/

#pragma once

#include <memory>
#include "Collective.h"
#include "Comm_op.h"
#include <vector>
#include <map>

namespace Tacos {

class NpuResult;

class LinkResult {
  public:
    enum class LinkType { Ingress, Egress };

    using LinkId = CommOp::LinkId;
    using OpId = CommOp::OpId;
    //using ChunkId = Tacos::ChunkId;

    LinkResult(LinkId linkId, LinkType type, NpuResult* npu) noexcept;

    [[nodiscard]] LinkId id() const noexcept;
    void send(ChunkId chunk) noexcept;
    void recv(ChunkId chunk) noexcept;

  [[nodiscard]] const std::map<OpId, CommOp>& ops() const noexcept;

  private:
    LinkId id_;

    OpId nextOpId_ = 0;
    [[nodiscard]] OpId currentOpId() noexcept;

    NpuResult& npu_;
    LinkType type_;

    std::map<OpId, CommOp> ops_;
};

}  // namespace tacos
