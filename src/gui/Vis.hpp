// ========================================================================== //
//                                                                            //
// Copyright (c) 2014-2020 The University of Texas at Austin.                 //
// All rights reserved.                                                       //
//                                                                            //
// Licensed under the Apache License, Version 2.0 (the "License");            //
// you may not use this file except in compliance with the License.           //
// A copy of the License is included with this software in the file LICENSE.  //
// If your copy does not contain the License, you may obtain a copy of the    //
// License at:                                                                //
//                                                                            //
//     https://www.apache.org/licenses/LICENSE-2.0                            //
//                                                                            //
// Unless required by applicable law or agreed to in writing, software        //
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT  //
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.           //
// See the License for the specific language governing permissions and        //
// limitations under the License.                                             //
//                                                                            //
// ========================================================================== //

#pragma once
  
#include <iostream>
#include "GxyData.hpp"

#include <QJsonArray>

class Vis : public GxyData
{
public:
  
  Vis() : GxyData() {}
  Vis(std::string o) : GxyData(o) {}

  QtNodes::NodeDataType type() const override
  { 
    return QtNodes::NodeDataType {"vis", "VIS"};
  }

  virtual void print() override
  {
    GxyPacket::print();
    std::cerr << "source file: " << source << "\n";
    std::cerr << "colormap file: " << colormap_file << "\n";
    std::cerr << "colormap range: " << cmap_range_min << " " << cmap_range_max << "\n";
  }

  virtual void toJson(QJsonObject& p) override
  {
    GxyPacket::toJson(p);

    p["colormap"] = colormap_file.c_str();
    p["dataset"] = source.c_str();

    QJsonArray dataRangeJson;
    dataRangeJson.push_back(cmap_range_min);
    dataRangeJson.push_back(cmap_range_max);
    p["data range"] = dataRangeJson;
  }

  virtual void fromJson(QJsonObject p) override
  {
    GxyPacket::fromJson(p);

    colormap_file = p["colormap"].toString().toStdString();
    cmap_range_min = p["data range"].toArray()[0].toDouble();
    cmap_range_max = p["data range"].toArray()[1].toDouble();
  }

  std::string source;
  std::string colormap_file;
  float cmap_range_min, cmap_range_max;
};
