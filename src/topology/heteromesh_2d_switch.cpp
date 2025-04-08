/******************************************************************************
This source code is licensed under the MIT license found in the
LICENSE file in the root directory of this source tree.

Copyright (c) 2022 Intel Corporation
Copyright (c) 2022 Georgia Institute of Technology
*******************************************************************************/

#include <cassert>
#include <tacos/topology/heteromesh_2d_switch.h>

using namespace tacos;

HeteroMesh2DSwitch::HeteroMesh2DSwitch(const int node_size,
               const int node_num,
               const Latency latency_0,
               const Bandwidth bandwidth_0,
               const Latency latency_1,
               const Bandwidth bandwidth_1) noexcept
    : node_size(node_size),
      node_num(node_num) {
    assert(node_size > 0);
    assert(node_num > 0);
    assert(latency_0 >= 0);
    assert(bandwidth_0 > 0);
    assert(latency_1 >= 0);
    assert(bandwidth_1 > 0);

    // compute GPUs count
    setNpusCount(node_size * node_num, node_num);

    int npuCount = node_size * node_num;

    // connect intra links
    for (auto h = 0; h < node_num; h++) {
        for (auto w1 = 0; w1 < node_size - 1; w1++) {
            for (auto w2 = w1 + 1; w2 < node_size; w2++) {
            //auto w2 = w1 + 1;
                const auto src = (h * node_size) + w1;
                const auto dest = (h * node_size) + w2;
                connect(src, dest, latency_0, bandwidth_0, true);
            }
        }
    }

    // connect inter links
    for (int s = 0; s < node_num; s++) {
        const auto src = npuCount + s;
        for (int d = 0; d < node_size; d++) {
            const auto dest = (s * node_size) + d;
            connect(src, dest, latency_1, bandwidth_1, true);
        }
    }
    for (int s1 = 0; s1 < node_num; s1++) {
        for (int s2 = s1 + 1; s2 < node_num; s2++) {
            const auto src = npuCount + s1;
            const auto dest = npuCount + s2;
            connect(src, dest, latency_1, bandwidth_1, true);
        }
    }

    setPath();
}
