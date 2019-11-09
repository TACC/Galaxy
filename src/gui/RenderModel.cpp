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

#include "RenderModel.hpp"

RenderModel::RenderModel() 
{
  QFrame *frame  = new QFrame();
  QVBoxLayout *layout = new QVBoxLayout();
  layout->setSpacing(0);
  layout->setContentsMargins(2, 0, 2, 0);
  frame->setLayout(layout);

  QPushButton *openCamera = new QPushButton("Camera");
  connect(openCamera, SIGNAL(released()), this, SLOT(openCameraDialog()));
  layout->addWidget(openCamera);

  QPushButton *openLights = new QPushButton("Lights");
  connect(openLights, SIGNAL(released()), this, SLOT(openLightsDialog()));
  layout->addWidget(openLights);
  
  _container->setCentralWidget(frame);

  renderWindow.show();

  QPushButton *open = new QPushButton("Open");
  connect(open, SIGNAL(released()), &renderWindow, SLOT(show()));
  _container->addButton(open);

  connect(this, SIGNAL(cameraChanged(Camera&)), &renderWindow, SLOT(onCameraChanged(Camera&)));
  connect(this, SIGNAL(lightingChanged(LightingEnvironment&)), &renderWindow, SLOT(onLightingChanged(LightingEnvironment&)));
}

unsigned int
RenderModel::nPorts(PortType portType) const
{
  if (portType == PortType::In)
    return 1;
  else
    return 0;
}

NodeDataType
RenderModel::dataType(PortType pt, PortIndex pi) const
{
  return GxyVis().type();
}

std::shared_ptr<NodeData>
RenderModel::outData(PortIndex)
{
  return NULL;
}

void
RenderModel::
setInData(std::shared_ptr<NodeData> data, PortIndex portIndex)
{
  input = std::dynamic_pointer_cast<GxyVis>(data);
  std::cerr << "RenderModel receives:\n";
  if (input) input->print();
  else std::cerr << "nothing\n";
}

NodeValidationState
RenderModel::validationState() const
{
  return NodeValidationState::Valid;
}

QString
RenderModel::validationMessage() const
{
  return QString("copacetic");
}
