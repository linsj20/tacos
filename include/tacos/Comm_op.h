/******************************************************************************
This source code is licensed under the MIT license found in the
LICENSE file in the root directory of this source tree.

Copyright (c) 2022 Intel Corporation
Copyright (c) 2022 Georgia Institute of Technology
*******************************************************************************/

#pragma once

#include "Collective.h"

namespace Tacos {

class CommOp {
  public:
    //using ChunkId = Tacos::ChunkId;
    using LinkId = int;
    using OpId = int;

    CommOp(ChunkId chunkId, LinkId linkId, OpId opId) noexcept;

    void setDepOp(CommOp* depOp);

    void setDepended();

    [[nodiscard]] ChunkId chunkId() const noexcept;

    [[nodiscard]] bool hasDep() const noexcept;

    [[nodiscard]] LinkId linkId() const noexcept;

    [[nodiscard]] OpId opId() const noexcept;

    [[nodiscard]] const CommOp* depOp() const noexcept;

    [[nodiscard]] bool depended() const noexcept;

  private:
    ChunkId chunkId_;
    LinkId linkId_;
    OpId opId_;

    bool hasDep_ = false;
    bool depended_ = false;
    CommOp* depOp_;
};

}  // namespace tacos
