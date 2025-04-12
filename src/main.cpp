/******************************************************************************
This source code is licensed under the MIT license found in the
LICENSE file in the root directory of this source tree.

Copyright (c) 2022 Intel Corporation
Copyright (c) 2022 Georgia Institute of Technology
*******************************************************************************/

#include <tacos/collective/all_gather.h>
#include <tacos/event-queue/timer.h>
#include <tacos/logger/logger.h>
#include <tacos/synthesizer/synthesizer.h>
#include <tacos/topology/heteromesh_2d_switch.h>
#include <tacos/writer/xml_writer.h>

using namespace tacos;

int main() {
    // initialize logger
    Logger::init("tacos.log");

    // construct a topology
    const auto node_size = 8;
    const auto node_num = 2;
    const auto intra_bandwidth = 25.0;  // GB/s
    const auto intra_latency = 1500;     // ns
    const auto inter_bandwidth = 300.0;  // GB/s
    const auto inter_latency = 500;     // ns

    const auto topology =
        std::make_shared<HeteroMesh2DSwitch>(node_size, node_num, intra_bandwidth, intra_latency, inter_bandwidth, inter_latency);
    const auto npusCount = node_size * node_num;

    Logger::info("Topology Information");
    Logger::info("\t", "- NPUs Count: ", npusCount);
    Logger::info();

    // target collective
    const auto chunkSize = 1'048'576;  // B
    const auto initChunksPerNpu = 1;

    const auto collective =
        std::make_shared<AllGather>(npusCount, chunkSize, initChunksPerNpu);
    const auto chunksCount = collective->getChunksCount();

    const auto chunkSizeMB = chunkSize / (1 << 20);
    Logger::info("Collective Information");
    Logger::info("\t", "- Chunks Count: ", chunksCount);
    Logger::info("\t", "- Chunk Size: ", chunkSizeMB, " MB");
    Logger::info();

    // instantiate synthesizer
    auto synthesizer = Synthesizer(topology, collective, true);

    // create timer
    auto timer = Timer();

    // synthesize collective algorithm
    Logger::info("Synthesis Process");

    timer.start();
    auto result = synthesizer.synthesize();
    timer.stop();

    Logger::info();

    // print result
    Logger::info("Synthesis Result");

    const auto collectiveTimePS = result.collectiveTime();
    const auto elapsedTimeUSec = timer.elapsedTime();
    const auto elapsedTimeSec = elapsedTimeUSec / 1e6;
    Logger::info("\t", "- Time to solve: ", elapsedTimeUSec, " us (",
                 elapsedTimeSec, " s)");

    const auto collectiveTimeUSec = collectiveTimePS / 1.0e6;
    Logger::info("\t", "- Synthesized Collective Time: ", collectiveTimePS,
                 " ps (", collectiveTimeUSec, " us)");
    Logger::info();

    // write into XML
    auto xmlWriter = XmlWriter("tacos.xml", topology, collective, result);
    xmlWriter.write();

    // terminate
    Logger::info("Done!");
    return 0;
}
