/******************************************************************************
This source code is licensed under the MIT license found in the
LICENSE file in the root directory of this source tree.
*******************************************************************************/

#include "HeteroMesh2D.h"
#include <cassert>

using namespace Tacos;

HeteroMesh2D::HeteroMesh2D(const int width, const int height, const LinkAlphaBeta linkAlphaBeta_0, const LinkAlphaBeta linkAlphaBeta_1) noexcept: Topology() {
    assert(width > 0);
    assert(height > 0);
    assert(linkAlphaBeta_0.first >= 0);
    assert(linkAlphaBeta_0.second >= 0);
    assert(linkAlphaBeta_1.first >= 0);
    assert(linkAlphaBeta_1.second >= 0);

    // compute NPUs count
    setNpusCount(width * height);

    // connect width-wise links
    for (auto h = 0; h < height; h++) {
        for (auto w1 = 0; w1 < width - 1; w1++) {
            for (auto w2 = w1 + 1; w2 < width; w2++) {
            //auto w2 = w1 + 1;
                const auto src = (h * width) + w1;
                const auto dest = (h * width) + w2;
                connect(src, dest, linkAlphaBeta_0, true);
            }
        }
    }

    // connect height-wise links
    for (auto h1 = 0; h1 < height - 1; h1++) {
        for (auto w1 = 0; w1 < width; w1++) {
            const auto src = (h1 * width) + w1;
            for (auto h2 = h1 + 1; h2 < height; h2++) {
                //for (auto w2 = 0; w2 < width; w2++) {
                auto w2 = w1;
                    const auto dest = (h2 * width) + w2;
                    connect(src, dest, linkAlphaBeta_1, true);
                //}
            }
        }
    }
}
