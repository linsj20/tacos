/******************************************************************************
This source code is licensed under the MIT license found in the
LICENSE file in the root directory of this source tree.
*******************************************************************************/

#pragma once

#include "Collective.h"
#include "EventQueue.h"
#include "TacosNetwork.h"
#include "Topology.h"
#include "Synthesis_result.h"
#include <array>
#include <memory>

namespace Tacos {
    class TacosGreedy {
    public:
        TacosGreedy(std::shared_ptr<Topology> topology, std::shared_ptr<Collective> collective) noexcept;

        [[nodiscard]] SynthesisResult solve() noexcept;

    private:
        using RequestSet = std::vector<std::pair<ChunkId, NpuId>>;
        using CandidateLinkSet = std::set<std::pair<LinkId, Time>>;
        using Contains = std::vector<std::vector<bool>>;

        std::unique_ptr<TacosNetwork> network;
        EventQueue eventQueue;

        std::shared_ptr<Topology> topology;
        std::shared_ptr<Collective> collective;

        SynthesisResult synthesisResult;

        int npusCount;
        int chunksCount;
        ChunkSize chunkSize;

        std::random_device randomDevice;
        std::default_random_engine randomEngine {randomDevice()};

        std::shared_ptr<TacosGreedy::RequestSet> initializeRequests(std::shared_ptr<Contains> contains) noexcept;

        bool prepareBacktracking(std::shared_ptr<RequestSet> requests, Time currentTime,
                                 std::shared_ptr<Contains> contains) noexcept;

        static std::pair<LinkId, Time> selectBestLink(const CandidateLinkSet& candidateLinks) noexcept;
    };
}  // namespace Tacos
