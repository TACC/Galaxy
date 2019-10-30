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

#include "TraceDecoratorModel.hpp"

TraceDecoratorModel::TraceDecoratorModel() 
{
  QFrame *frame  = new QFrame();
  QGridLayout *layout = new QGridLayout();
  frame->setLayout(layout);

  layout->addWidget(new QLabel("head time"), 0, 0);

  headtime = new QLineEdit();
  headtime->setText(QString::number(0.0));
  headtime->setValidator(new QDoubleValidator());
  layout->addWidget(headtime, 0, 1);

  layout->addWidget(new QLabel("delta-t"), 1, 0);

  deltat = new QLineEdit();
  deltat->setText(QString::number(0.1)); 
  deltat->setValidator(new QDoubleValidator());
  layout->addWidget(deltat, 1, 1);

  _container->setCentralWidget(frame);
}

unsigned int
TraceDecoratorModel::nPorts(PortType portType) const
{
  if (portType == PortType::In)
    return 1;
  else
    return 1;
}

NodeDataType
TraceDecoratorModel::dataType(PortType pt, PortIndex) const
{
  if (pt == PortType::In)
    return GxyStreamTraces().type();
  else
    return GxyData().type();
}

void
TraceDecoratorModel::apply() { std::cerr << "Apply\n"; }

std::shared_ptr<NodeData>
TraceDecoratorModel::outData(PortIndex)
{
  std::shared_ptr<GxyStreamTraces> result;
  return std::static_pointer_cast<NodeData>(result);
}

void
TraceDecoratorModel::
setInData(std::shared_ptr<NodeData> data, PortIndex portIndex)
{
  // volumeData = std::dynamic_pointer_cast<GxyData>(data);
}


NodeValidationState
TraceDecoratorModel::validationState() const
{
  return NodeValidationState::Valid;
}


QString
TraceDecoratorModel::validationMessage() const
{
  return QString("copacetic");
}

