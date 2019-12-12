// ========================================================================== //
// Copyright (c) 2014-2019 The University of Texas at Austin.                 //
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
  
#include <nodes/NodeDataModel>
#include <QJsonArray>

#include "GxyPacket.hpp"

struct GxyDataInfo
{
  GxyDataInfo()
  {
    name = "none";
    type = -1;
    isVector = true;
    data_min = data_max = 0;
    box[0] = box[1] = box[2] = box[3] = box[4] = box[5] = 0;
  }

  virtual void toJson(QJsonObject& p) 
  {
    p["dataset"] = QString(name.c_str());
    p["type"] = type;
    p["isVector"] = isVector;
    QJsonArray rangeJson = { data_min, data_max };
    p["data range"] = rangeJson;

    for (auto i = 0; i < 6; i++)
      p["box"].toArray().push_back(box[i]);
  }

  virtual void fromJson(QJsonObject p)
  {
    name = p["dataset"].toString().toStdString();
    type = p["type"].toInt();
    isVector = p["isVector"].toBool();
    data_min = p["data range"].toArray()[0].toDouble();
    data_max = p["data range"].toArray()[1].toDouble();
    for (auto i = 0; i < 6; i++)
      box[i] = p["box"].toArray()[i].toDouble();
  }

  void print()
  {
    std::cerr << "name: " << name << "\n";
    std::cerr << "key: " << key << "\n";
    std::cerr << "type: " << type << "\n";
    std::cerr << "isVector: " << isVector << "\n";
    std::cerr << "range: " << data_min << " " << data_max << "\n";
    std::cerr << "box: " << box[0] << " " << box[1] << " " << box[2] << " " << box[3] << " " << box[4] << " " << box[5] << "\n";
  }

  std::string name;
  int key;
  int type;
  bool isVector;
  float data_min, data_max;
  float box[6];
};

class GxyData : public GxyPacket
{
public:
  GxyData(GxyDataInfo& di) : dataInfo(di), GxyPacket() {}
  GxyData() : GxyPacket() {}

  GxyData(std::string o) : GxyPacket(o) {}
  
  void print() override
  {
    GxyPacket::print();
    dataInfo.print();
  }

  bool isValid()
  {
    if (!GxyPacket::isValid()) 
    {
      std::cerr << "GxyPacket::isValid = NO\n";
      return false;
    }

    if (dataInfo.name == "" && dataInfo.key == -1)
    {
      std::cerr << "GxyData::isValid ... dataInfo.name is NULL and dataInfo.key == -1\n";
      return false;
    }
    
    return true;
  }

  QtNodes::NodeDataType type() const override
  {
    return QtNodes::NodeDataType {"gxydata", "GxyData"};
  }

  virtual void toJson(QJsonObject& p) override
  {
    GxyPacket::toJson(p);
    dataInfo.toJson(p);
  }

  virtual void fromJson(QJsonObject p) override
  {
    GxyPacket::fromJson(p);
    dataInfo.fromJson(p);
  }

  GxyDataInfo dataInfo;
};
