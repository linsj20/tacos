/******************************************************************************
This source code is licensed under the MIT license found in the
LICENSE file in the root directory of this source tree.

Copyright (c) 2022-2025 Intel Corporation
Copyright (c) 2022-2025 Georgia Institute of Technology
*******************************************************************************/

#include <iostream>
#include <tacos/collective/all_gather.h>
#include <tacos/event_queue/timer.h>
#include <tacos/synthesizer/synthesizer.h>
#include <tacos/topology/hetero_mesh_2d.h>
#include <tacos/writer/xml_writer.h>
#include <tacos/writer/xml_transformer.h>

using namespace tacos;

int main() {
    // set print precision
    fixed(std::cout);
    std::cout.precision(2);

    // construct a topology
    const auto width = 8;
    const auto height = 4;

    // bandwidth and latency settings
    const auto bandwidth_0 = 125;  // GB/s
    const auto latency_0 = 700;     // ns
    const auto bandwidth_1 = 12.0 / 8;  // GB/s
    const auto latency_1 = 1700;     // ns

    // convert to GiB/s and microseconds
    const auto bandwidthWidth = bandwidth_0 / 1.024 / 1.024;  // GiB/s
    const auto latencyWidth = latency_0 / 1000.0;  // us
    const auto bandwidthHeight = bandwidth_1 / 1.024 / 1.024;  // GiB/s
    const auto latencyHeight = latency_1 / 1000.0;  // us

    const auto topology = HeteroMesh2D(width, height, 
                                       bandwidthWidth, latencyWidth,
                                       bandwidthHeight, latencyHeight);
    const auto npusCount = topology.npusCount();
    std::cout << "NPUs count: " << npusCount << std::endl;

    // create collective
    const auto collectiveSize = 128.0;  // MB
    const auto initChunks = 1;  // initial #chunks per NPU
    const auto chunkSize = (collectiveSize * 1024 * 1024) / (initChunks * npusCount);  // bytes per chunk
    const auto collective = AllGather(npusCount, initChunks);
    const auto chunksCount = collective.chunksCount();
    std::cout << "Chunks count: " << chunksCount << std::endl;

    // create timer
    auto synthesizerTimer = Timer();

    // create synthesizer and solve
    synthesizerTimer.start();
    auto synthesizer = Synthesizer();
    auto synthesisResult = synthesizer.solve(topology, collective, chunkSize);
    synthesizerTimer.stop();

    // print result
    auto time = synthesizerTimer.time();
    auto collectiveTime = synthesisResult.collectiveTime();
    std::cout << std::endl;
    std::cout << "Time to solve: " << time / 1000 << " ms" << std::endl;
    std::cout << "All-Gather Time: " << collectiveTime << " us" << std::endl;
    std::cout << "All-Reduce Time: " << collectiveTime * 2 << " us" << std::endl;

    // write XML output
    auto xmlWriter = XmlWriter("tacos.xml", topology, collective, synthesisResult);
    xmlWriter.write();

    // transform XML with cp_times = 2
    auto xmlTransformer = XmlTransformer("tacos.xml", "tacos_transformed.xml", 2);
    xmlTransformer.transform();

    // terminate
    return 0;
}
