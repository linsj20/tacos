/******************************************************************************
This source code is licensed under the MIT license found in the
LICENSE file in the root directory of this source tree.

Copyright (c) 2022 Intel Corporation
Copyright (c) 2022 Georgia Institute of Technology
*******************************************************************************/

#include <algorithm>
#include <cassert>
#include <cerrno>
#include <iterator>
#include <limits>
#include <tacos/logger/logger.h>
#include <tacos/synthesizer/synthesizer.h>
#include <cstdio>

using namespace tacos;

Synthesizer::Synthesizer(const std::shared_ptr<Topology> topology,
                         const std::shared_ptr<Collective> collective,
                         const bool verbose) noexcept
    : topology(topology),
      collective(collective),
      ten(topology),
      verbose(verbose),
      synthesisResult(topology, collective) {
    assert(topology != nullptr);
    assert(collective != nullptr);

    npusCount = topology->getNpusCount();
    chunksCount = collective->getChunksCount();

    // set topology chunk size
    const auto chunkSize = collective->getChunkSize();
    topology->setChunkSize(chunkSize);
    distinctLinkDelays = topology->getDistinctLinkDelays();

    // setup initial precondition and postcondition
    precondition = collective->getPrecondition();
    postcondition = collective->getPostcondition();

    // remove already-satisfied postconditions
    processInitialPostcondition();

    // setup initial events
    currentTime = eventQueue.getCurrentTime();
    eventQueue.schedule(1);
}

SynthesisResult Synthesizer::synthesize() noexcept {
    int i = 0;
    while (true) {
        // run link-chunk matching
        linkChunkMatching();

        // if synthesis is completed, break
        if (synthesisCompleted()) {
            break;
        }

        // if synthesis is not finished, schedule next events
        if (eventQueue.empty()) {
            scheduleNextEvents();
        }

        // update current time
        currentTime = eventQueue.pop();

        // update TEN current time
        printf("========= Update Time [%ld] ========\n", currentTime);
        auto finished = ten.updateCurrentTime(currentTime);
        for (auto& [src, dest, chunk] : *finished) {
            markLinkChunkMatch(src, dest, chunk);
        }

        i++;
        if (i >= 200) {exit(0);}
    }

    assert(synthesisCompleted());

    synthesisResult.collectiveTime(currentTime);
    return synthesisResult;
}

void Synthesizer::scheduleNextEvents() noexcept {
    /*
    assert(!distinctLinkDelays.empty());

    for (const auto linkDelay : distinctLinkDelays) {
        const auto nextEventTime = currentTime + linkDelay;
        eventQueue.schedule(nextEventTime);
    }
    */
    const auto nextTime = ten.nextTime();
    printf("NextTime: %ld\n", nextTime);
    const auto nextEventTime = nextTime;
    eventQueue.schedule(nextEventTime);
}

void Synthesizer::linkChunkMatching() noexcept {
    // get current precondition and postcondition
    const auto currentPrecondition = precondition;
    auto currentPostcondition = postcondition;
    for (auto& [a, b] : precondition) {
        printf("[%d]: ", a);
        for (auto& c : b) {
            printf("%d ", c);
        }
        printf("\n");
    }

    // iterate over all unsatisfied postconditions
    while (!currentPostcondition.empty()) {
        // randomly select one postcondition
        const auto [dest, chunk] = selectPostcondition(&currentPostcondition);

        // backtrack the TEN to find potential source NPUs
        const auto sourceNpus = ten.backtrackTEN(dest, chunk);

        if (sourceNpus.empty()) {
            continue;
        }

        // among the sourceNpus, find the candidate sources
        const auto candidateSourceNpus =
            checkCandidateSourceNpus(chunk, currentPrecondition, sourceNpus);

        // if there are no candidate source NPUs, skip
        if (candidateSourceNpus.empty()) {
            continue;
        }

        // randomly select one candidate source NPU
        auto [path, bw] = selectSourceNpu(candidateSourceNpus);

        ten.assignPath(path, chunk);
        postcondition[dest].erase(chunk);
        if (postcondition[dest].size() == 0) {
            postcondition.erase(dest);
        }

        // link-chunk match made: mark this
        //markLinkChunkMatch(src, dest, chunk);
    }
}

std::pair<Synthesizer::NpuID, Synthesizer::ChunkID> Synthesizer::
    selectPostcondition(
        CollectiveCondition* const currentPostcondition) noexcept {
    assert(currentPostcondition != nullptr);
    assert(!currentPostcondition->empty());

    // randomly pick an entry
    auto postconditionDist =
        std::uniform_int_distribution<>(0, currentPostcondition->size() - 1);
    int randomNpuIdx = postconditionDist(randomEngine);
    auto randomNpuIt = std::next(currentPostcondition->begin(), randomNpuIdx);
    auto dest = randomNpuIt->first;
    auto& chunks = randomNpuIt->second;

    // randomly pick a chunk
    auto chunkDist = std::uniform_int_distribution<>(0, chunks.size() - 1);
    int randomChunkIdx = chunkDist(randomEngine);
    auto randomChunkIt = std::next(chunks.begin(), randomChunkIdx);
    auto chunk = *randomChunkIt;

    // remove selected chunk from the postcondition
    chunks.erase(randomChunkIt);

    // remove the selected npu, if there's no remaining postcondition
    if (chunks.empty()) {
        currentPostcondition->erase(randomNpuIt);
    }

    // return the selected npu and chunk
    return {dest, chunk};
}

std::set<std::pair<const Path *, Topology::Bandwidth>> Synthesizer::checkCandidateSourceNpus(
    const ChunkID chunk,
    const CollectiveCondition& currentPrecondition,
    const std::set<std::pair<const Path *, Bandwidth>>& sourceNpus) noexcept {
    assert(0 <= chunk && chunk < chunksCount);
    assert(!currentPrecondition.empty());
    assert(!sourceNpus.empty());

    auto candidateSourceNpus = std::set<std::pair<const Path*, Bandwidth>>();

    // check which source NPUs hold the chunk
    for (const auto& [p, bw] : sourceNpus) {
        const auto chunksAtSrc = currentPrecondition.at(p->path->front());
        if (chunksAtSrc.find(chunk) != chunksAtSrc.end()) {
            candidateSourceNpus.insert(std::pair<const Path*, Bandwidth>(p, bw));
        }
    }

    return candidateSourceNpus;
}

std::pair<const Path *, Synthesizer::Bandwidth> Synthesizer::selectSourceNpu(
    const std::set<std::pair<const Path *, Bandwidth>>& candidateSourceNpus) noexcept {
    assert(!candidateSourceNpus.empty());

    // if only one candidate source NPU, return it
    if (candidateSourceNpus.size() == 1) {
        const auto firstCandidate = candidateSourceNpus.begin();
        return *firstCandidate;
    }
    
    std::set<std::pair<const Path *, Bandwidth>> bestSourceNpus;
    Bandwidth bestBandwidth = std::numeric_limits<Bandwidth>::min();
    for (const auto& [p, bw] : candidateSourceNpus) {
        if (bw > bestBandwidth) {
            bestBandwidth = bw;
        }
    }
    for (const auto& pair : candidateSourceNpus) {
        if (pair.second == bestBandwidth) {
            bestSourceNpus.insert(pair);
        }
    }

    return *bestSourceNpus.begin();

    // randomly select one candidate source NPU
    auto bestSourceNpusDist =
        std::uniform_int_distribution<>(0, bestSourceNpus.size() - 1);
    int randomSrcIdx = bestSourceNpusDist(randomEngine);
    auto randomSrcIt = std::next(bestSourceNpus.begin(), randomSrcIdx);
    return *randomSrcIt;
}

void Synthesizer::markLinkChunkMatch(const NpuID src,
                                     const NpuID dest,
                                     const ChunkID chunk) noexcept {
    // mark the link-chunk match
    if (verbose) {
        Logger::info("At EventTime ", currentTime, " ps: ");
        Logger::info("\t", "Chunk ", chunk, ": ", src, " -> ", dest);
    }

    // FIXME: link-chunk match made here: chunk, src -> dst
    synthesisResult.npu(src).linkTo(dest).send(chunk);
    synthesisResult.npu(dest).linkFrom(src).recv(chunk);

    // insert the chunk to the precondition
    precondition[dest].insert(chunk);

    // remove the chunk from the postcondition
    //postcondition[dest].erase(chunk);

    // if there's no remaining postcondition of the dest, remove it
    if (postcondition[dest].empty()) {
        postcondition.erase(dest);
    }
}

bool Synthesizer::synthesisCompleted() const noexcept {
    // synthesis is done when there's no remaining postcondition
    return postcondition.empty() && ten.complete();
}

void Synthesizer::processInitialPostcondition() noexcept {
    // remove precondition from postcondition
    for (const auto& [src, chunks] : precondition) {
        for (const auto chunk : chunks) {
            postcondition[src].erase(chunk);
        }
    }
}
