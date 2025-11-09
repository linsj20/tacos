/******************************************************************************
This source code is licensed under the MIT license found in the
LICENSE file in the root directory of this source tree.

Copyright (c) 2022-2025 Intel Corporation
Copyright (c) 2022-2025 Georgia Institute of Technology
*******************************************************************************/

#include <cassert>
#include <tacos/topology/hetero_mesh_2d.h>

using namespace tacos;

HeteroMesh2D::HeteroMesh2D(const int width,
                           const int height,
                           const Bandwidth bandwidthWidth,
                           const Latency latencyWidth,
                           const Bandwidth bandwidthHeight,
                           const Latency latencyHeight) noexcept
    : Topology() {
    assert(width > 0);
    assert(height > 0);
    assert(bandwidthWidth > 0);
    assert(latencyWidth >= 0);
    assert(bandwidthHeight > 0);
    assert(latencyHeight >= 0);

    setNpusCount_(width * height);

    // connect width-wise links (all-to-all within each row)
    for (auto h = 0; h < height; h++) {
        for (auto w1 = 0; w1 < width - 1; w1++) {
            for (auto w2 = w1 + 1; w2 < width; w2++) {
                const auto src = (h * width) + w1;
                const auto dest = (h * width) + w2;
                connect_(src, dest, bandwidthWidth, latencyWidth, true);
            }
        }
    }

    // connect height-wise links (connect each column vertically)
    for (auto h1 = 0; h1 < height - 1; h1++) {
        for (auto w = 0; w < width; w++) {
            const auto src = (h1 * width) + w;
            for (auto h2 = h1 + 1; h2 < height; h2++) {
                const auto dest = (h2 * width) + w;
                connect_(src, dest, bandwidthHeight, latencyHeight, true);
            }
        }
    }
}
