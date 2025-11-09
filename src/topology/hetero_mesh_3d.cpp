/******************************************************************************
This source code is licensed under the MIT license found in the
LICENSE file in the root directory of this source tree.

Copyright (c) 2022-2025 Intel Corporation
Copyright (c) 2022-2025 Georgia Institute of Technology
*******************************************************************************/

#include <cassert>
#include <tacos/topology/hetero_mesh_3d.h>

using namespace tacos;

HeteroMesh3D::HeteroMesh3D(const int sizeX,
                           const int sizeY,
                           const int sizeZ,
                           const Bandwidth bandwidthX,
                           const Latency latencyX,
                           const Bandwidth bandwidthY,
                           const Latency latencyY,
                           const Bandwidth bandwidthZ,
                           const Latency latencyZ) noexcept
    : Topology() {
    assert(sizeX > 0);
    assert(sizeY > 0);
    assert(sizeZ > 0);
    assert(bandwidthX > 0);
    assert(latencyX >= 0);
    assert(bandwidthY > 0);
    assert(latencyY >= 0);
    assert(bandwidthZ > 0);
    assert(latencyZ >= 0);

    setNpusCount_(sizeX * sizeY * sizeZ);

    // connect x-wise links (all-to-all within each x-row)
    for (auto z = 0; z < sizeZ; z++) {
        for (auto y = 0; y < sizeY; y++) {
            for (auto x1 = 0; x1 < sizeX - 1; x1++) {
                for (auto x2 = x1 + 1; x2 < sizeX; x2++) {
                    const auto src = (z * sizeX * sizeY) + (y * sizeX) + x1;
                    const auto dest = (z * sizeX * sizeY) + (y * sizeX) + x2;
                    connect_(src, dest, bandwidthX, latencyX, true);
                }
            }
        }
    }

    // connect y-wise links (connect along y-direction)
    for (auto z = 0; z < sizeZ; z++) {
        for (auto y1 = 0; y1 < sizeY - 1; y1++) {
            for (auto x = 0; x < sizeX; x++) {
                const auto src = (z * sizeX * sizeY) + (y1 * sizeX) + x;
                for (auto y2 = y1 + 1; y2 < sizeY; y2++) {
                    const auto dest = (z * sizeX * sizeY) + (y2 * sizeX) + x;
                    connect_(src, dest, bandwidthY, latencyY, true);
                }
            }
        }
    }

    // connect z-wise links (connect along z-direction)
    for (auto z1 = 0; z1 < sizeZ - 1; z1++) {
        for (auto y = 0; y < sizeY; y++) {
            for (auto x = 0; x < sizeX; x++) {
                const auto src = (z1 * sizeX * sizeY) + (y * sizeX) + x;
                for (auto z2 = z1 + 1; z2 < sizeZ; z2++) {
                    const auto dest = (z2 * sizeX * sizeY) + (y * sizeX) + x;
                    connect_(src, dest, bandwidthZ, latencyZ, true);
                }
            }
        }
    }
}
