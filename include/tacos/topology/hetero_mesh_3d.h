/******************************************************************************
This source code is licensed under the MIT license found in the
LICENSE file in the root directory of this source tree.

Copyright (c) 2022-2025 Intel Corporation
Copyright (c) 2022-2025 Georgia Institute of Technology
*******************************************************************************/

#pragma once

#include <tacos/topology/topology.h>

namespace tacos {

/// @brief 3D heterogeneous mesh topology with different bandwidth/latency for x, y, and z links
class HeteroMesh3D final : public Topology {
  public:
    /// @brief Construct a 3D heterogeneous mesh topology
    /// @param sizeX size in x-direction (width)
    /// @param sizeY size in y-direction (height)
    /// @param sizeZ size in z-direction (depth)
    /// @param bandwidthX bandwidth of x-wise links (in GiB/sec)
    /// @param latencyX latency of x-wise links (in microseconds)
    /// @param bandwidthY bandwidth of y-wise links (in GiB/sec)
    /// @param latencyY latency of y-wise links (in microseconds)
    /// @param bandwidthZ bandwidth of z-wise links (in GiB/sec)
    /// @param latencyZ latency of z-wise links (in microseconds)
    HeteroMesh3D(int sizeX,
                 int sizeY,
                 int sizeZ,
                 Bandwidth bandwidthX,
                 Latency latencyX,
                 Bandwidth bandwidthY,
                 Latency latencyY,
                 Bandwidth bandwidthZ,
                 Latency latencyZ) noexcept;
};

}  // namespace tacos
