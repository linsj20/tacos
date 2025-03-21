/******************************************************************************
This source code is licensed under the MIT license found in the
LICENSE file in the root directory of this source tree.

Copyright (c) 2022 Intel Corporation
Copyright (c) 2022 Georgia Institute of Technology
*******************************************************************************/

#pragma once

#include <tacos/topology/topology.h>

namespace tacos {
class Torus2D final : public Topology {
  public:
    Torus2D(int width,
            int height,
            Latency latency,
            Bandwidth bandwidth) noexcept;

  private:
    int width;
    int height;
};

}  // namespace tacos
