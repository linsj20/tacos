/******************************************************************************
This source code is licensed under the MIT license found in the
LICENSE file in the root directory of this source tree.

Copyright (c) 2022-2025 Intel Corporation
Copyright (c) 2022-2025 Georgia Institute of Technology
*******************************************************************************/

#include <iostream>
#include <tacos/writer/xml_writer.h>

using namespace tacos;

XmlWriter::XmlWriter(const std::string& filename,
                     const Topology& topology,
                     const Collective& collective,
                     SynthesisResult& synthesisResult) noexcept
    : path(filename), topology_(topology), collective_(collective), synthesisResult_(synthesisResult) {}

void XmlWriter::write() noexcept {
    writeAlgo();
    for (auto npu = 0; npu < topology_.npusCount(); npu++) {
        writeNpu(npu);
    }
    save();
}

void XmlWriter::writeAlgo() noexcept {
    algo = xml.append_child("algo");
    algo.append_attribute("name") = "tacos";
    algo.append_attribute("proto") = "Simple";
    algo.append_attribute("nchannels") = 1;
    algo.append_attribute("nchunksperloop") = collective_.chunksCount();
    algo.append_attribute("ngpus") = topology_.npusCount();
    algo.append_attribute("coll") = "allgather";
    algo.append_attribute("inplace") = 1;
    algo.append_attribute("outofplace") = 0;
    algo.append_attribute("minBytes") = 0;
    algo.append_attribute("maxBytes") = 0;
}

void XmlWriter::writeNpu(NpuID npuId) noexcept {
    auto npu = algo.append_child("gpu");
    npu.append_attribute("id") = npuId;
    npu.append_attribute("i_chunks") = 0;
    npu.append_attribute("o_chunks") = collective_.chunksCount();
    npu.append_attribute("s_chunks") = 0;
    for (const auto& [src, link] : synthesisResult_.npu(npuId).ingressLinks()) {
        writeIngressLink(npu, npuId, src, link);
    }
    for (const auto& [dest, link] : synthesisResult_.npu(npuId).egressLinks()) {
        writeEgressLink(npu, npuId, dest, link);
    }
}

void XmlWriter::save() noexcept {
    if (xml.save_file(path.c_str(), PUGIXML_TEXT("\t"), pugi::format_no_declaration)) {
        std::cout << "XML file written at: " << path << std::endl;
    } else {
        std::cout << "XML file writing failed" << std::endl;
    }
}

void XmlWriter::writeIngressLink(pugi::xml_node& gpu, NpuID npu, NpuID src, const LinkResult& link) noexcept {
    auto tb = gpu.append_child("tb");
    tb.append_attribute("id") = link.id();
    tb.append_attribute("send") = -1;
    tb.append_attribute("recv") = src;
    tb.append_attribute("chan") = 0;
    for (const auto& [opId, op] : link.ops()) {
        const auto chunkId = op.chunkId();
        auto step = tb.append_child("step");
        step.append_attribute("s") = opId;
        step.append_attribute("type") = "r";
        step.append_attribute("srcbuf") = "o";
        step.append_attribute("srcoff") = chunkId;
        step.append_attribute("dstbuf") = "o";
        step.append_attribute("dstoff") = chunkId;
        step.append_attribute("cnt") = 1;
        step.append_attribute("depid") = -1;
        step.append_attribute("deps") = -1;
        step.append_attribute("hasdep") = op.depended() ? 1 : 0;
    }
}

void XmlWriter::writeEgressLink(pugi::xml_node& gpu, NpuID npu, NpuID dest, const LinkResult& link) noexcept {
    auto tb = gpu.append_child("tb");
    tb.append_attribute("id") = link.id();
    tb.append_attribute("send") = dest;
    tb.append_attribute("recv") = -1;
    tb.append_attribute("chan") = 0;
    for (const auto& [opId, op] : link.ops()) {
        const auto chunkId = op.chunkId();
        auto step = tb.append_child("step");
        step.append_attribute("s") = opId;
        step.append_attribute("type") = "s";
        step.append_attribute("srcbuf") = "o";
        step.append_attribute("srcoff") = chunkId;
        step.append_attribute("dstbuf") = "o";
        step.append_attribute("dstoff") = chunkId;
        step.append_attribute("cnt") = 1;
        if (op.hasDep()) {
            step.append_attribute("depid") = op.depOp()->linkId();
            step.append_attribute("deps") = op.depOp()->opId();
        } else {
            step.append_attribute("depid") = -1;
            step.append_attribute("deps") = -1;
        }
        step.append_attribute("hasdep") = op.depended() ? 1 : 0;
    }
}
