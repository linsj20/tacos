/******************************************************************************
This source code is licensed under the MIT license found in the
LICENSE file in the root directory of this source tree.
*******************************************************************************/

#include "AllGather.h"
#include "HeteroMesh2D.h"
#include "TacosGreedy.h"
#include "Timer.h"
#include "Logger.h"
#include "Xml_writer.h"
#include <iostream>


using namespace Tacos;

int main() {
    // set print precision
    fixed(std::cout);
    std::cout.precision(2);

    // construct a topology
    const auto width = 4;
    const auto height = 2;
    /*
    const auto bandwidth_0 = 15;  // GB/s
    const auto latency_0 = 700;     // ns
    const auto bandwidth_1 = 15;  // GB/s
    const auto latency_1 = 700;     // ns
    */
    const auto bandwidth_0 = 125 / 8;  // GB/s
    const auto latency_0 = 700;     // ns
    const auto bandwidth_1 = 12.0 / 8 / 8;  // GB/s
    const auto latency_1 = 1700;     // ns

    const auto beta_0 = 1'000'000 / (bandwidth_0 * 1024.0);  // us/MiB
    const auto beta_1 = 1'000'000 / (bandwidth_1 * 1024.0);  // us/MiB
    const auto linkAlphaBeta_0 = std::make_pair(latency_0, beta_0);
    const auto linkAlphaBeta_1 = std::make_pair(latency_1, beta_1);

    const auto topology = std::make_shared<HeteroMesh2D>(width, height, linkAlphaBeta_0, linkAlphaBeta_1);
    const auto npusCount = topology->getNpusCount();
    std::cout << "NPUs count: " << npusCount << std::endl;

    // create collective
    const auto collectiveSize = 1.0;  // MB
    const auto initChunks = 1;  // initial #chunks per NPU
    const auto chunkSize = collectiveSize / (initChunks * npusCount);  // bytes per chunk
    const auto collective = std::make_shared<AllGather>(npusCount, chunkSize, initChunks);
    const auto chunksCount = collective->getChunksCount();
    std::cout << "Chunks count: " << chunksCount << std::endl;

    // create timer
    auto solverTimer = Timer("PathSolver");

    // create solver and solve
    solverTimer.start();
    auto solver = TacosGreedy(topology, collective);
    auto result = solver.solve();
    solverTimer.stop();

    // print result
    auto time = solverTimer.getTime("ms");
    auto collectiveTime = result.collectiveTime();
    std::cout << std::endl;
    std::cout << "Time to solve: " << time << " ms" << std::endl;
    std::cout << "All-Gather Time: " << collectiveTime << " us" << std::endl;
    std::cout << "All-Reduce Time: " << collectiveTime * 2 << " us" << std::endl;

    auto xmlWriter = XmlWriter("tacos.xml", topology, collective, result);
    xmlWriter.write();

    // terminate
    return 0;
}
