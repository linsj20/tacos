/******************************************************************************
This source code is licensed under the MIT license found in the
LICENSE file in the root directory of this source tree.
*******************************************************************************/

#pragma once

#include "Topology.h"

namespace Tacos {
    class HeteroMesh2D final : public Topology {
    public:
        HeteroMesh2D(int width, int height, LinkAlphaBeta linkAlphaBeta_0, LinkAlphaBeta linkAlphaBeta_1) noexcept;
    };
}  // namespace Tacos
