/******************************************************************************
This source code is licensed under the MIT license found in the
LICENSE file in the root directory of this source tree.

Copyright (c) 2022-2025 Intel Corporation
Copyright (c) 2022-2025 Georgia Institute of Technology
*******************************************************************************/

#pragma once

#include <memory>
#include <pugixml.hpp>
#include <string>
#include <tacos/collective/collective.h>
#include <tacos/topology/topology.h>
#include <tacos/writer/synthesis_result.h>

namespace tacos {

class XmlWriter {
  public:
    using NpuID = Topology::NpuID;

    XmlWriter(const std::string& filename,
              const Topology& topology,
              const Collective& collective,
              SynthesisResult& synthesisResult) noexcept;

    void write() noexcept;

  private:
    pugi::xml_document xml;
    pugi::xml_node algo;
    std::string path;
    const Topology& topology_;
    const Collective& collective_;
    SynthesisResult& synthesisResult_;

    void writeAlgo() noexcept;
    void writeNpu(NpuID npuId) noexcept;
    void writeIngressLink(pugi::xml_node& gpu, NpuID npu, NpuID src, const LinkResult& link) noexcept;
    void writeEgressLink(pugi::xml_node& gpu, NpuID npu, NpuID dest, const LinkResult& link) noexcept;
    void save() noexcept;
};

}  // namespace tacos
