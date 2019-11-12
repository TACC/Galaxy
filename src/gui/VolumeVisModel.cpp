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
  output = std::make_shared<VolumeVis>(getModelIdentifier());

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

  volumeRender = new QCheckBox("Volume render?");
  volumeRender->setChecked(false);
  layout->addWidget(volumeRender);

  QFrame *cmap_box = new QFrame();
  QHBoxLayout *cmap_box_layout = new QHBoxLayout();
  cmap_box->setLayout(cmap_box_layout);

  cmap_box_layout->addWidget(new QLabel("tfunc"));

  tf_widget = new QLineEdit();
  cmap_box_layout->addWidget(tf_widget);
  
  QPushButton *tfunc_browse_button = new QPushButton("...");
  cmap_box_layout->addWidget(tfunc_browse_button);
  connect(tfunc_browse_button, SIGNAL(released()), this, SLOT(openFileSelectorDialog()));

  layout->addWidget(cmap_box);

  _container->setCentralWidget(frame);

  connect(_container->getApplyButton(), SIGNAL(released()), this, SLOT(onApply()));
  
}

void 
VolumeVisModel::onApply()
{
  if (input)
  {
    output->dataName = input->dataName;
    output->dataType = input->dataType;
    
    output->print();

    Q_EMIT dataUpdated(0);
  }
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
    return GxyVis().type();
}

std::shared_ptr<NodeData>
VolumeVisModel::outData(PortIndex)
{
  return std::static_pointer_cast<NodeData>(output);
}

void
VolumeVisModel::
setInData(std::shared_ptr<NodeData> data, PortIndex portIndex)
{
  input = std::dynamic_pointer_cast<GxyData>(data);
  std::cerr << "VolumeVisModel receives:\n";
  if (input)
  {
    std::cerr << "VVM setInData... get = " << ((long)input.get()) << "\n";
    input->print();
  }
  else std::cerr << "nothing\n";
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

QJsonObject
VolumeVisModel::save() const 
{
  QJsonObject modelJson = NodeDataModel::save();
  output->save(modelJson);
  return modelJson;
}

void
VolumeVisModel::restore(QJsonObject const &p)
{
  output->restore(p);
  volumeRender->setChecked(output->volume_rendering_flag);
  tf_widget->setText(output->transfer_function.c_str());
}

