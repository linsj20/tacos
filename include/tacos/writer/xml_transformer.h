/******************************************************************************
This source code is licensed under the MIT license found in the
LICENSE file in the root directory of this source tree.

Copyright (c) 2022-2025 Intel Corporation
Copyright (c) 2022-2025 Georgia Institute of Technology
*******************************************************************************/

#pragma once

#include <pugixml.hpp>
#include <string>

namespace tacos {

class XmlTransformer {
  public:
    XmlTransformer(const std::string& inputFile, 
                   const std::string& outputFile, 
                   int cpTimes = 2) noexcept;

    bool transform() noexcept;

  private:
    std::string inputFile_;
    std::string outputFile_;
    int cpTimes_;

    void transformRoot_(pugi::xml_node& root) noexcept;
    void transformGpu_(pugi::xml_node& gpu) noexcept;
    void transformTb_(pugi::xml_node& tb, pugi::xml_node& gpu, int tbIndex) noexcept;
};

}  // namespace tacos
