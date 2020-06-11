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
  output = std::dynamic_pointer_cast<Vis>(std::shared_ptr<VolumeVis>(new VolumeVis(model_identifier)));
  output->setValid(false);

  isovaluesDialog = new ScalarsDialog;
  slicesDialog = new PlanesDialog;

  QFrame *frame  = new QFrame;
  QVBoxLayout *layout = new QVBoxLayout;
  layout->setSpacing(0);
  layout->setContentsMargins(2, 0, 2, 0);
  frame->setLayout(layout);

  volumeRender = new QCheckBox("Volume render?");
  connect(volumeRender, SIGNAL(stateChanged(int)), this, SLOT(volume_rendering_flag_state_changed(int)));
  volumeRender->setChecked(false);
  layout->addWidget(volumeRender);

  geomWidgets = new QFrame;
  QVBoxLayout *l = new QVBoxLayout;
  geomWidgets->setLayout(l);

  QPushButton *openPlanes = new QPushButton("Planes");
  connect(openPlanes, SIGNAL(released()), this, SLOT(openPlanesDialog()));
  l->addWidget(openPlanes);

  QPushButton *openIsovalues = new QPushButton("Isovalues");
  connect(openIsovalues, SIGNAL(released()), this, SLOT(openIsovaluesDialog()));
  l->addWidget(openIsovalues);

  layout->addWidget(geomWidgets);

  _properties->addProperties(frame);

  connect(_properties->getApplyButton(), SIGNAL(released()), this, SLOT(onApply()));
}

VolumeVisModel::~VolumeVisModel()
{
  delete isovaluesDialog;
  delete slicesDialog;
}

void
VolumeVisModel::loadInputDrivenWidgets(std::shared_ptr<GxyPacket> o) 
{
  if (input)
    VisModel::loadInputDrivenWidgets(input);
}

void
VolumeVisModel::loadParameterWidgets() const
{
  VisModel::loadParameterWidgets();

  std::shared_ptr<VolumeVis> v = std::dynamic_pointer_cast<VolumeVis>(output);

  slicesDialog->clear();
  slicesDialog->set_planes(v->slices);

  isovaluesDialog->clear();
  isovaluesDialog->set_scalars(v->isovalues);

  volumeRender->setChecked(v->volume_rendering_flag);
}

void
VolumeVisModel::loadOutput(std::shared_ptr<GxyPacket> o) const
{
  VisModel::loadOutput(o);

  std::shared_ptr<VolumeVis> v = std::dynamic_pointer_cast<VolumeVis>(o);

  if (volumeRender->isChecked())
  {
    v->slices.clear();
    v->isovalues.clear();
    v->volume_rendering_flag = true;
  }
  else
  {
    v->slices = slicesDialog->get_planes();
    v->isovalues = isovaluesDialog->get_scalars();
    v->volume_rendering_flag = false;
  }
}

void 
VolumeVisModel::onApply()
{
  output = std::shared_ptr<VolumeVis>(new VolumeVis(model_identifier));
  loadOutput(std::dynamic_pointer_cast<GxyPacket>(output));
  output->setValid(true);

  GxyModel::onApply();
}

unsigned int
VolumeVisModel::nPorts(PortType portType) const
{
  return 1; // PortType::In or ::Out
}

QtNodes::NodeDataType
VolumeVisModel::dataType(PortType pt, PortIndex) const
{
  if (pt == PortType::In)
    return GxyData().type();
  else
    return Vis().type();
}

std::shared_ptr<NodeData>
VolumeVisModel::outData(PortIndex)
{
  return std::static_pointer_cast<NodeData>(output);
}

void
VolumeVisModel::setInData(std::shared_ptr<NodeData> data, PortIndex portIndex)
{
  input = std::dynamic_pointer_cast<GxyData>(data);
  if (input)
  {
    loadInputDrivenWidgets(std::dynamic_pointer_cast<GxyPacket>(input));
    loadOutput(std::dynamic_pointer_cast<GxyPacket>(output));
    output->setValid(isValid());
    VisModel::setInData(data, portIndex);
    if (isValid())
      onApply();
  }
}

void 
VolumeVisModel::onDataRangeReset()
{
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
  loadOutput(output);

  QJsonObject modelJson = VisModel::save();

  loadOutput(std::dynamic_pointer_cast<GxyPacket>(output));
  output->toJson(modelJson);

  return modelJson;
}

void
VolumeVisModel::restore(QJsonObject const &p)
{
  output->fromJson(p);
  loadParameterWidgets();
}

bool
VolumeVisModel::isValid()
{
  if (! VisModel::isValid() || ! input->isValid())
  {
    return false;
  }

  return true;
}
