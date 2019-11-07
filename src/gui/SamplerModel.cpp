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

#include "SamplerModel.hpp"

SamplerModel::SamplerModel() 
{
  QFrame *frame  = new QFrame();
  QVBoxLayout *layout = new QVBoxLayout();
  layout->setSpacing(0);
  layout->setContentsMargins(2, 0, 2, 0);
  frame->setLayout(layout);

  QPushButton *openCamera = new QPushButton("Camera");
  connect(openCamera, SIGNAL(released()), this, SLOT(openCameraDialog()));
  layout->addWidget(openCamera);

  QWidget *algorithm = new QWidget();
  QGridLayout *algorithm_layout = new QGridLayout();
  algorithm->setLayout(algorithm_layout);

  algorithm_layout->addWidget(new QLabel("algorithm"), 0, 0);
  
  type = new QComboBox();
  type->addItem("gradient");
  type->addItem("isosurface");

  algorithm_layout->addWidget(type, 0, 1);

  algorithm_layout->addWidget(new QLabel("parameter"), 1, 0);

  parameter = new QLineEdit();
  parameter->setFixedWidth(100);
  parameter->setValidator(new QDoubleValidator());
  parameter->setText(QString::number(0.0));
  algorithm_layout->addWidget(parameter, 1, 1);

  layout->addWidget(algorithm);

  _container->setCentralWidget(frame);
}

unsigned int
SamplerModel::nPorts(QtNodes::PortType portType) const
{
  if (portType == QtNodes::PortType::In)
    return 1;
  else
    return 1;
}

QtNodes::NodeDataType
SamplerModel::dataType(QtNodes::PortType pt, QtNodes::PortIndex pi) const
{
  return GxyData().type();
}

void
SamplerModel::apply() { std::cerr << "Apply\n"; }

std::shared_ptr<QtNodes::NodeData>
SamplerModel::outData(QtNodes::PortIndex)
{
  std::shared_ptr<GxyData> result;
  return std::static_pointer_cast<QtNodes::NodeData>(result);
}

void
SamplerModel::
setInData(std::shared_ptr<QtNodes::NodeData> data, QtNodes::PortIndex portIndex)
{
}


QtNodes::NodeValidationState
SamplerModel::validationState() const
{
  return QtNodes::NodeValidationState::Valid;
}


QString
SamplerModel::validationMessage() const
{
  return QString("copacetic");
}

