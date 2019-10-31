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

#include "VolumeVisModel.hpp"

VolumeVisModel::VolumeVisModel() 
{
  QFrame *frame  = new QFrame();
  QVBoxLayout *layout = new QVBoxLayout();
  layout->setSpacing(0);
  layout->setContentsMargins(2, 0, 2, 0);
  frame->setLayout(layout);

  QPushButton *openPlanes = new QPushButton("Planes");
  connect(openPlanes, SIGNAL(released()), this, SLOT(openPlanesDialog()));
  layout->addWidget(openPlanes);

  QPushButton *openIsovalues = new QPushButton("Isovalues");
  connect(openIsovalues, SIGNAL(released()), this, SLOT(openIsovaluesDialog()));
  layout->addWidget(openIsovalues);

  QCheckBox *volumeRender = new QCheckBox("Volume render?");
  volumeRender->setChecked(false);
  layout->addWidget(volumeRender);

  QFrame *cmap_box = new QFrame();
  QHBoxLayout *cmap_box_layout = new QHBoxLayout();
  cmap_box->setLayout(cmap_box_layout);

  cmap_box_layout->addWidget(new QLabel("tfunc"));

  tf_widget = new QLineEdit();
  cmap_box_layout->addWidget(tf_widget);
  // cmap_box_layout->addWidget(&tf_widget);
  
  QPushButton *tfunc_browse_button = new QPushButton("...");
  cmap_box_layout->addWidget(tfunc_browse_button);
  connect(tfunc_browse_button, SIGNAL(released()), this, SLOT(openFileSelectorDialog()));

  layout->addWidget(cmap_box);
  
  _container->setCentralWidget(frame);
}

unsigned int
VolumeVisModel::nPorts(PortType portType) const
{
  return 1; // PortType::In or ::Out
}

NodeDataType
VolumeVisModel::dataType(PortType pt, PortIndex) const
{
  if (pt == PortType::In)
    return GxyData().type();
  else
    return JsonVis().type();
}

void
VolumeVisModel::apply() { std::cerr << "Apply\n"; }

std::shared_ptr<NodeData>
VolumeVisModel::outData(PortIndex)
{
  std::shared_ptr<Json> result;
  return std::static_pointer_cast<NodeData>(result);
}

void
VolumeVisModel::
setInData(std::shared_ptr<NodeData> data, PortIndex portIndex)
{
  volumeData = std::dynamic_pointer_cast<GxyData>(data);
}


NodeValidationState
VolumeVisModel::validationState() const
{
  return NodeValidationState::Valid;
}


QString
VolumeVisModel::validationMessage() const
{
  return QString("copacetic");
}
