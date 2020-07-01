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
  
#include <nodes/NodeDataModel>
#include <QJsonArray>

#include "GxyPacket.hpp"

class GxyInt : public GxyPacket
{
public:
  GxyInt(int i) : value(i), GxyPacket() {}
  GxyInt() : GxyPacket() {}

  GxyInt(std::string o) : GxyPacket(o) {}
  
  void print() override
  {
    GxyPacket::print();
    std::cerr << "IntValue: " << value << "\n";
  }

  bool isValid() { return GxyPacket::isValid(); }

  QtNodes::NodeDataType type() const override
  {
    return QtNodes::NodeDataType {"gxyint", "GxyInt"};
  }

  virtual void toJson(QJsonObject& p) override
  {
    GxyPacket::toJson(p);
    p["value"] = value;
  }

  virtual void fromJson(QJsonObject p) override
  {
    GxyPacket::fromJson(p);
    value = p["value"].toInt();
  }

  void setValue(int i) { value = i; }
  int getValue() { return value; }

private:
  int value;
};

class GxyDouble : public GxyPacket
{
public:
  GxyDouble(double d) : value(d), GxyPacket() {}
  GxyDouble() : GxyPacket() {}

  GxyDouble(std::string o) : GxyPacket(o) {}
  
  void print() override
  {
    GxyPacket::print();
    std::cerr << "DoubleValue: " << value << "\n";
  }

  bool isValid() { return GxyPacket::isValid(); }

  QtNodes::NodeDataType type() const override
  {
    return QtNodes::NodeDataType {"gxydbl", "GxyDbl"};
  }

  virtual void toJson(QJsonObject& p) override
  {
    GxyPacket::toJson(p);
    p["value"] = value;
  }

  virtual void fromJson(QJsonObject p) override
  {
    GxyPacket::fromJson(p);
    value = p["value"].toDouble();
  }

  void setValue(double i) { value = i; }
  double getValue() { return value; }

private:
  double value;
};

