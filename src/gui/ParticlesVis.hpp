// ========================================================================== //
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
#include "Vis.hpp"

class ParticlesVis : public Vis
{
public:
  
  ParticlesVis() : Vis() {}
  ParticlesVis(std::string o) : Vis(o) {}

  QtNodes::NodeDataType type() const override
  { 
    return QtNodes::NodeDataType {"ptvis", "PTVIS"};
  }

  virtual void print() override
  {
    Vis::print();
    std::cerr << "radius map range: " << minrange << " " << maxrange << "\n";
    std::cerr << "radius map value: " << minradius << " " << maxradius << "\n";
  }

  void toJson(QJsonObject& p) override
  {
    Vis::toJson(p);

    p["type"] = "ParticlesVis";
    p["radius0"] = minradius;
    p["radius1"] = maxradius;
    p["value0"] = minrange;
    p["value1"] = maxrange;
  }

  void fromJson(QJsonObject p) override
  {
    Vis::fromJson(p);

    minradius = p["radius0"].toDouble();
    maxradius = p["radius1"].toDouble();
    minrange = p["value0"].toDouble();
    maxrange = p["value1"].toDouble();
  }

  float minrange;
  float maxrange;
  float minradius;
  float maxradius;
};

