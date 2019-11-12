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

class GxyGuiObject : public QtNodes::NodeData
{
public:
  
  GxyGuiObject() {}
  GxyGuiObject(std::string o) { origin = o; }
  
  QtNodes::NodeDataType type() const override
  { 
    return QtNodes::NodeDataType {"gxygui", "GxyGui"};
  }

  virtual void print()
  {
    std::cerr << "origin: " << origin << "\n";
  }

  std::string get_origin() { return origin; }

private:

  std::string origin;
};


class GxyData : public GxyGuiObject
{
public:
  GxyData() : GxyGuiObject() {}

  GxyData(std::string o) : GxyGuiObject(o) {}
  
  virtual void 
  print()
  {
    GxyGuiObject::print();
    std::cerr << "dataName: " << dataName << "\n";
    std::cerr << "dataType: " << dataType << "\n";
  }

  virtual void save(QJsonObject& p) const
  {
    p["dataset"] = QString(dataName.c_str());
  }

  virtual void restore(QJsonObject const &p)
  {
    dataName = p["dataset"].toString().toStdString();
  }


  std::string dataName;
  int dataType;
};
