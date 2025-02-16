/******************************************************************************
This source code is licensed under the MIT license found in the
LICENSE file in the root directory of this source tree.

Copyright (c) 2022 Intel Corporation
Copyright (c) 2022 Georgia Institute of Technology
*******************************************************************************/

#pragma once

#include <map>
#include "Collective.h"
#include "Topology.h"
#include "Comm_op.h"
#include "Link_result.h"

namespace Tacos {

class NpuResult {
  public:
    //using NpuId = Topology::NpuId;
    //using ChunkId = Collective::ChunkId;
    using NpuId = Tacos::NpuId;
    using ChunkId = Tacos::ChunkId;
    using LinkType = LinkResult::LinkType;
    using LinkId = LinkResult::LinkId;
    using OpId = LinkResult::OpId;

    NpuResult(NpuId id, std::shared_ptr<Topology> topology) noexcept;

    [[nodiscard]] LinkResult& linkFrom(NpuId id) noexcept;
    [[nodiscard]] LinkResult& linkTo(NpuId id) noexcept;

    [[nodiscard]] const std::map<NpuId, LinkResult>& ingressLinks() const noexcept;
    [[nodiscard]] const std::map<NpuId, LinkResult>& egressLinks() const noexcept;

    void registerRecvDep(ChunkId chunk, CommOp* depOp) noexcept;

    CommOp* const getDep(ChunkId chunk) noexcept;

  private:
    int npusCount_;

    NpuId id_;

    LinkId nextLinkId_ = 0;

    std::map<ChunkId, CommOp*> depRecvOp_;

    std::map<NpuId, LinkResult> ingressLinks_;
    std::map<NpuId, LinkResult> egressLinks_;
};

}  // namespace tacos
