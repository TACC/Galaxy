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

#include "InterpolatorModel.hpp"

InterpolatorModel::InterpolatorModel() 
{
  QFrame *frame  = new QFrame();
  QGridLayout *layout = new QGridLayout();
  frame->setLayout(layout);

  _container->setCentralWidget(frame);
}

unsigned int
InterpolatorModel::nPorts(PortType portType) const
{
  if (portType == PortType::In)
    return 2;
  else
    return 1;
}

NodeDataType
InterpolatorModel::dataType(PortType pt, PortIndex) const
{
  if (pt == PortType::In)
    return GxyData().type();
  else
    return GxyData().type();
}

void
InterpolatorModel::apply() { std::cerr << "Apply\n"; }

std::shared_ptr<NodeData>
InterpolatorModel::outData(PortIndex)
{
  std::shared_ptr<GxyStreamTraces> result;
  return std::static_pointer_cast<NodeData>(result);
}

void
InterpolatorModel::
setInData(std::shared_ptr<NodeData> data, PortIndex portIndex)
{
  // volumeData = std::dynamic_pointer_cast<GxyData>(data);
}


NodeValidationState
InterpolatorModel::validationState() const
{
  return NodeValidationState::Valid;
}


QString
InterpolatorModel::validationMessage() const
{
  return QString("copacetic");
}

