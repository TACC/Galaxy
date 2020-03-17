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

#include <iostream>
#include <nodes/NodeDataModel>
#include <QJsonDocument>
  
class GxyPacket : public QtNodes::NodeData
{
public:
  
  GxyPacket() { }
  GxyPacket(std::string o) { origin = o; }
  
  QtNodes::NodeDataType type() const override
  { 
    return QtNodes::NodeDataType {"pkt", "PKT"};
  }

  virtual void print()
  {
    std::cerr << "origin: " << origin << "\n";
  }

  std::string get_origin() { return origin; }

  void setValid(bool v) { valid = v; }
  bool isValid() { return valid; }

  virtual void toJson(QJsonObject& o)
  {
    o["origin"] = origin.c_str();
  }

  virtual void fromJson(QJsonObject o)
  {
    origin = o["origin"].toString().toStdString();
    valid = false;
  }

private:

  bool valid = false;
  std::string origin;
};


