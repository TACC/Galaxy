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

#include <QtCore/QUuid>

using QtNodes::NodeDataType;
using QtNodes::NodeData;

class GxyData : public NodeData
{
public:
  
  GxyData() 
  {
    QUuid uuid = QUuid::createUuid();
    _name = uuid.toString();
  }
  
  GxyData(char *name) : _name(name)
  {}
  
  NodeDataType type() const override
  { 
    return NodeDataType {"galaxy data", "GxyData"};
  }
  
  virtual QString name() { return _name; }

private:

  QString _name;
};

class GxyStreamTraces : public GxyData
{
public:
  
  NodeDataType type() const override
  { 
    return NodeDataType {"strace", "GxyStreamTrace"};
  }
};

