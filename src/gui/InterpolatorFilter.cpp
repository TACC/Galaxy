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

#include "InterpolatorFilter.hpp"

InterpolatorFilter::InterpolatorFilter() 
{
  QFrame *frame  = new QFrame();
  QGridLayout *layout = new QGridLayout();
  frame->setLayout(layout);

  _properties->addProperties(frame);
}

unsigned int
InterpolatorFilter::nPorts(QtNodes::PortType portType) const
{
  if (portType == QtNodes::PortType::In)
    return 2;
  else
    return 1;
}

QtNodes::NodeDataType
InterpolatorFilter::dataType(QtNodes::PortType pt, QtNodes::PortIndex) const
{
  if (pt == QtNodes::PortType::In)
    return GxyData().type();
  else
    return GxyData().type();
}

void
InterpolatorFilter::apply() { std::cerr << "Apply\n"; }

std::shared_ptr<QtNodes::NodeData>
InterpolatorFilter::outData(QtNodes::PortIndex)
{
  std::shared_ptr<GxyData> result;
  return std::static_pointer_cast<QtNodes::NodeData>(result);
}

// void
// InterpolatorFilter::
// setInData(std::shared_ptr<QtNodes::NodeData> data, QtNodes::PortIndex portIndex)
// {
  // GxyFilter::setInData(data, portIndex);
  // if (isValid())
    // Q_EMIT dataUpdated(0);
// }


QtNodes::NodeValidationState
InterpolatorFilter::validationState() const
{
  return QtNodes::NodeValidationState::Valid;
}


QString
InterpolatorFilter::validationMessage() const
{
  return QString("copacetic");
}

