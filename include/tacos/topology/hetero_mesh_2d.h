/******************************************************************************
This source code is licensed under the MIT license found in the
LICENSE file in the root directory of this source tree.

Copyright (c) 2022-2025 Intel Corporation
Copyright (c) 2022-2025 Georgia Institute of Technology
*******************************************************************************/

#pragma once

#include <tacos/topology/topology.h>

namespace tacos {

/// @brief 2D heterogeneous mesh topology with different bandwidth/latency for width and height links
class HeteroMesh2D final : public Topology {
  public:
    /// @brief Construct a 2D heterogeneous mesh topology
    /// @param width width of the mesh (number of NPUs in x-direction)
    /// @param height height of the mesh (number of NPUs in y-direction)
    /// @param bandwidthWidth bandwidth of width-wise links (in GiB/sec)
    /// @param latencyWidth latency of width-wise links (in microseconds)
    /// @param bandwidthHeight bandwidth of height-wise links (in GiB/sec)
    /// @param latencyHeight latency of height-wise links (in microseconds)
    HeteroMesh2D(int width,
                 int height,
                 Bandwidth bandwidthWidth,
                 Latency latencyWidth,
                 Bandwidth bandwidthHeight,
                 Latency latencyHeight) noexcept;
};

}  // namespace tacos
