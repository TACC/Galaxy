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

  layout->addWidget(new QLabel("max steps"), 0, 0);

  maxsteps = new QLineEdit();
  maxsteps->setText(QString::number(2000));
  maxsteps->setValidator(new QIntValidator());
  layout->addWidget(maxsteps, 0, 1);

  layout->addWidget(new QLabel("step size"), 1, 0);

  stepsize = new QLineEdit();
  stepsize->setText(QString::number(0.2)); 
  stepsize->setValidator(new QDoubleValidator());
  layout->addWidget(stepsize, 1, 1);

  layout->addWidget(new QLabel("min velocity"), 2, 0);

  minvelocity = new QLineEdit();
  minvelocity->setText(QString::number(1e-12));
  minvelocity->setValidator(new QDoubleValidator());
  layout->addWidget(minvelocity, 2, 1);

  layout->addWidget(new QLabel("max time"), 3, 0);

  maxtime = new QLineEdit();
  maxtime->setText(QString::number(-1.0));
  maxtime->setValidator(new QDoubleValidator());
  layout->addWidget(maxtime, 3, 1);

  _properties->setPropertiesWidget(frame);
}

unsigned int
TraceDecoratorModel::nPorts(QtNodes::PortType portType) const
{
  if (portType == QtNodes::PortType::In)
    return 2;
  else
    return 1;
}

QtNodes::NodeDataType
TraceDecoratorModel::dataType(QtNodes::PortType pt, QtNodes::PortIndex) const
{
  if (pt == QtNodes::PortType::In)
    return GxyData().type();
  else
    return GxyData().type();
}

void
TraceDecoratorModel::apply() { std::cerr << "Apply\n"; }

std::shared_ptr<QtNodes::NodeData>
TraceDecoratorModel::outData(QtNodes::PortIndex)
{
  std::shared_ptr<GxyData> result;
  return std::static_pointer_cast<QtNodes::NodeData>(result);
}

void
TraceDecoratorModel::
setInData(std::shared_ptr<QtNodes::NodeData> data, QtNodes::PortIndex portIndex)
{
  // volumeData = std::dynamic_pointer_cast<GxyData>(data);
}


QtNodes::NodeValidationState
TraceDecoratorModel::validationState() const
{
  return QtNodes::NodeValidationState::Valid;
}


QString
TraceDecoratorModel::validationMessage() const
{
  return QString("copacetic");
}

