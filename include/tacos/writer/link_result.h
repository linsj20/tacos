/******************************************************************************
This source code is licensed under the MIT license found in the
LICENSE file in the root directory of this source tree.

Copyright (c) 2022-2025 Intel Corporation
Copyright (c) 2022-2025 Georgia Institute of Technology
*******************************************************************************/

#pragma once

#include <map>
#include <tacos/collective/collective.h>
#include <tacos/writer/comm_op.h>

namespace tacos {

class NpuResult;

class LinkResult {
  public:
    enum class LinkType { Ingress, Egress };

    using ChunkID = Collective::ChunkID;
    using LinkID = CommOp::LinkID;
    using OpID = CommOp::OpID;

    LinkResult(LinkID linkId, LinkType type, NpuResult* npu) noexcept;

    [[nodiscard]] LinkID id() const noexcept;
    void send(ChunkID chunk) noexcept;
    void recv(ChunkID chunk) noexcept;
    [[nodiscard]] const std::map<OpID, CommOp>& ops() const noexcept;

  private:
    LinkID id_;
    OpID nextOpId_ = 0;
    NpuResult& npu_;
    LinkType type_;
    std::map<OpID, CommOp> ops_;

    [[nodiscard]] OpID currentOpId() noexcept;
};

}  // namespace tacos
