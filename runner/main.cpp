/******************************************************************************
This source code is licensed under the MIT license found in the
LICENSE file in the root directory of this source tree.
*******************************************************************************/

#include "AllGather.h"
#include "Mesh2D.h"
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
    const auto width = 3;
    const auto height = 3;
    const auto latency = 0.5;  // us
    const auto bandwidth = 50;  // GiB/s

    const auto beta = 1'000'000 / (bandwidth * 1024.0);  // us/MiB
    const auto linkAlphaBeta = std::make_pair(latency, beta);

    const auto topology = std::make_shared<Mesh2D>(width, height, linkAlphaBeta);
    const auto npusCount = topology->getNpusCount();
    std::cout << "NPUs count: " << npusCount << std::endl;

    // create collective
    const auto collectiveSize = 1.0;  // MB
    const auto initChunks = 4;  // initial #chunks per NPU
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
