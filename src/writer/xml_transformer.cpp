/******************************************************************************
This source code is licensed under the MIT license found in the
LICENSE file in the root directory of this source tree.

Copyright (c) 2022-2025 Intel Corporation
Copyright (c) 2022-2025 Georgia Institute of Technology
*******************************************************************************/

#include <iostream>
#include <vector>
#include <tacos/writer/xml_transformer.h>

using namespace tacos;

struct OriginalStepValues {
    int srcoff;
    int dstoff;
    int depid;
};

static pugi::xml_node create_new_tb_block(
    const pugi::xml_node& modified_tb,
    int original_id,
    int original_chan,
    const std::vector<OriginalStepValues>& original_step_values,
    int cp_times,
    int offset)
{
    pugi::xml_node parent = modified_tb.parent();
    pugi::xml_node new_tb = parent.insert_child_after("tb", modified_tb);
    
    for (auto attr : modified_tb.attributes()) {
        std::string name = attr.name();
        if (name == "id") {
            new_tb.append_attribute("id").set_value(original_id * cp_times + offset);
        } else if (name == "chan") {
            new_tb.append_attribute("chan").set_value(original_chan + offset);
        } else {
            new_tb.append_attribute(name.c_str()).set_value(attr.value());
        }
    }
    
    if (!modified_tb.attribute("chan")) {
        new_tb.append_attribute("chan").set_value(offset);
    }
    
    int step_idx = 0;
    for (auto step : modified_tb.children("step")) {
        pugi::xml_node new_step = new_tb.append_child("step");
        
        for (auto attr : step.attributes()) {
            std::string name = attr.name();
            if (name == "srcoff") {
                new_step.append_attribute("srcoff").set_value(
                    original_step_values[step_idx].srcoff * cp_times + offset);
            } else if (name == "dstoff") {
                new_step.append_attribute("dstoff").set_value(
                    original_step_values[step_idx].dstoff * cp_times + offset);
            } else if (name == "depid" && original_step_values[step_idx].depid != -1) {
                new_step.append_attribute("depid").set_value(
                    original_step_values[step_idx].depid * cp_times + offset);
            } else {
                new_step.append_attribute(name.c_str()).set_value(attr.value());
            }
        }
        step_idx++;
    }
    
    return new_tb;
}

XmlTransformer::XmlTransformer(const std::string& inputFile,
                               const std::string& outputFile,
                               int cpTimes) noexcept
    : inputFile_(inputFile), outputFile_(outputFile), cpTimes_(cpTimes) {}

bool XmlTransformer::transform() noexcept {
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(inputFile_.c_str());
    
    if (!result) {
        std::cout << "XML parsing failed: " << result.description() << std::endl;
        return false;
    }

    auto root = doc.document_element();
    transformRoot_(root);

    for (auto gpu : root.children("gpu")) {
        transformGpu_(gpu);
    }

    if (doc.save_file(outputFile_.c_str(), PUGIXML_TEXT("\t"), pugi::format_no_declaration)) {
        std::cout << "Successfully transformed " << inputFile_ << " to " << outputFile_ << std::endl;
        return true;
    }

    std::cout << "XML file writing failed" << std::endl;
    return false;
}

void XmlTransformer::transformRoot_(pugi::xml_node& root) noexcept {
    if (auto attr = root.attribute("nchannels")) {
        attr.set_value(attr.as_int() * cpTimes_);
    }
    if (auto attr = root.attribute("nchunksperloop")) {
        attr.set_value(attr.as_int() * cpTimes_);
    }
    if (auto attr = root.attribute("nthreadblocks")) {
        attr.set_value(attr.as_int() * cpTimes_);
    }
}

void XmlTransformer::transformGpu_(pugi::xml_node& gpu) noexcept {
    if (auto attr = gpu.attribute("o_chunks")) {
        attr.set_value(attr.as_int() * cpTimes_);
    }
    std::vector<pugi::xml_node> tb_blocks;
    for (auto tb : gpu.children("tb")) {
        tb_blocks.push_back(tb);
    }

    for (int i = 0; i < static_cast<int>(tb_blocks.size()); i++) {
        transformTb_(tb_blocks[i], gpu, i);
    }
}

void XmlTransformer::transformTb_(pugi::xml_node& tb, pugi::xml_node& gpu, int tbIndex) noexcept {
    int original_id = tb.attribute("id").as_int();
    int original_chan = tb.attribute("chan") ? tb.attribute("chan").as_int() : 0;
    
    std::vector<OriginalStepValues> original_step_values;
    for (auto step : tb.children("step")) {
        OriginalStepValues vals;
        vals.srcoff = step.attribute("srcoff") ? step.attribute("srcoff").as_int() : 0;
        vals.dstoff = step.attribute("dstoff") ? step.attribute("dstoff").as_int() : 0;
        vals.depid = step.attribute("depid") ? step.attribute("depid").as_int() : -1;
        original_step_values.push_back(vals);
    }
    
    tb.attribute("id").set_value(original_id * cpTimes_);
    
    for (auto step : tb.children("step")) {
        if (step.attribute("srcoff")) {
            int val = step.attribute("srcoff").as_int();
            step.attribute("srcoff").set_value(val * cpTimes_);
        }
        if (step.attribute("dstoff")) {
            int val = step.attribute("dstoff").as_int();
            step.attribute("dstoff").set_value(val * cpTimes_);
        }
        if (step.attribute("depid") && step.attribute("depid").as_int() != -1) {
            int val = step.attribute("depid").as_int();
            step.attribute("depid").set_value(val * cpTimes_);
        }
    }
    
    pugi::xml_node last_created = tb;
    for (int offset = 1; offset < cpTimes_; ++offset) {
        last_created = create_new_tb_block(last_created, original_id, original_chan,
                                            original_step_values, cpTimes_, offset);
    }
}
