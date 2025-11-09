/******************************************************************************
This source code is licensed under the MIT license found in the
LICENSE file in the root directory of this source tree.

Copyright (c) 2022-2025 Intel Corporation
Copyright (c) 2022-2025 Georgia Institute of Technology
*******************************************************************************/

#pragma once

#include <tacos/collective/collective.h>

namespace tacos {

class CommOp {
  public:
    using ChunkID = Collective::ChunkID;
    using LinkID = int;
    using OpID = int;

    CommOp(ChunkID chunkId, LinkID linkId, OpID opId) noexcept;

    void setDepOp(CommOp* depOp);
    void setDepended();

    [[nodiscard]] ChunkID chunkId() const noexcept;
    [[nodiscard]] bool hasDep() const noexcept;
    [[nodiscard]] LinkID linkId() const noexcept;
    [[nodiscard]] OpID opId() const noexcept;
    [[nodiscard]] const CommOp* depOp() const noexcept;
    [[nodiscard]] bool depended() const noexcept;

  private:
    ChunkID chunkId_;
    LinkID linkId_;
    OpID opId_;
    bool hasDep_ = false;
    bool depended_ = false;
    CommOp* depOp_;
};

}  // namespace tacos
