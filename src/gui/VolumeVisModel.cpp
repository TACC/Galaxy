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

  connect(_container->getApplyButton(), SIGNAL(released()), this, SLOT(onApply()));
  
}

void 
VolumeVisModel::onApply()
{
  // std::cerr << "VVM onApply\n";
  // std::cerr << "input... get = " << ((long)input.get()) << "\n";
  // input->print();

  output->dataName = input->dataName;
  output->dataType = input->dataType;
  output->slices = slices;
  output->isovalues = isovalues;
  output->volume_rendering_flag = volume_rendering_flag;
  output->transfer_function = transfer_function;
  
  std::cerr << "output... get = " << ((long)output.get()) << "\n";
  output->print();

  Q_EMIT dataUpdated(0);
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

  QJsonArray isovaluesJson;

  for (auto isoval : isovalues)
    isovaluesJson.push_back(isoval);

  modelJson["isovalues"] = isovaluesJson;

  QJsonArray slicesJson;

  for (auto slice : slices)
  {
    QJsonObject sliceJson;
    sliceJson["a"] = slice.x;
    sliceJson["b"] = slice.y;
    sliceJson["c"] = slice.z;
    sliceJson["d"] = slice.w;
    slicesJson.push_back(sliceJson);
  }

  modelJson["slices"] = slicesJson;

  modelJson["volume rendering"] = volume_rendering_flag ? 1 : 0;

  modelJson["transfer function"] = QString::fromStdString(transfer_function);
  
  return modelJson;
}

void
VolumeVisModel::restore(QJsonObject const &p)
{
  QJsonArray isovaluesJson = p["isovalues"].toArray();

  for (auto isovalueJson : isovaluesJson) 
    isovalues.push_back(isovalueJson.toDouble());

  QJsonArray slicesJson = p["slices"].toArray();

  for (auto sliceJson : slicesJson) 
  {
    QJsonObject o = sliceJson.toObject();
    gxy::vec4f slice;
    slice.x = o["a"].toDouble();
    slice.y = o["b"].toDouble();
    slice.z = o["c"].toDouble();
    slice.w = o["d"].toDouble();
    slices.push_back(slice);
  }

  volume_rendering_flag = p["volume rendering"].toInt();
  transfer_function = p["transfer function"].toString().toStdString();

}

