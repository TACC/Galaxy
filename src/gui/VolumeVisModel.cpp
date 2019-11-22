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
  connect(volumeRender, SIGNAL(stateChanged(int)), this, SLOT(volume_rendering_flag_state_changed(int)));
  volumeRender->setChecked(false);
  layout->addWidget(volumeRender);

  QFrame *cmap_box = new QFrame();
  QHBoxLayout *cmap_box_layout = new QHBoxLayout();
  cmap_box->setLayout(cmap_box_layout);

  cmap_box_layout->addWidget(new QLabel("color map"));

  tf_widget = new QLineEdit();
  cmap_box_layout->addWidget(tf_widget);
  
  QPushButton *tfunc_browse_button = new QPushButton("...");
  cmap_box_layout->addWidget(tfunc_browse_button);
  connect(tfunc_browse_button, SIGNAL(released()), this, SLOT(openFileSelectorDialog()));

  layout->addWidget(cmap_box);

  QFrame *data_range_w = new QFrame;
  QHBoxLayout *data_range_l = new QHBoxLayout();
  data_range_l->setSpacing(0);
  data_range_l->setContentsMargins(2, 0, 2, 0);
  data_range_w->setLayout(data_range_l);

  data_range_l->addWidget(new QLabel("data range"));
  
  data_range_min = new QLineEdit;
  data_range_min->setText("0");
  data_range_min->setValidator(new QDoubleValidator());
  data_range_l->addWidget(data_range_min);
  
  data_range_max = new QLineEdit;
  data_range_max->setText("0");
  data_range_max->setValidator(new QDoubleValidator());
  data_range_l->addWidget(data_range_max);

  QPushButton *resetDataRange = new QPushButton("reset");
  connect(resetDataRange, SIGNAL(released()), this, SLOT(onDataRangeReset()));
  data_range_l->addWidget(resetDataRange);

  layout->addWidget(data_range_w);

  _container->setCentralWidget(frame);

  connect(data_range_max, SIGNAL(editingFinished()), this, SLOT(enableApply()));
  connect(data_range_min, SIGNAL(editingFinished()), this, SLOT(enableApply()));
  connect(_container->getApplyButton(), SIGNAL(released()), this, SLOT(onApply()));
}

void 
VolumeVisModel::onApply()
{
  if (input)
  {
    output->di.data_min = data_range_min->text().toDouble();
    output->di.data_max = data_range_max->text().toDouble();
    output->transfer_function = tf_widget->text().toStdString();
    output->volume_rendering_flag = volumeRender->isChecked();

    std::cerr << "output:\n";
    output->di.print();
    
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
VolumeVisModel::setInData(std::shared_ptr<NodeData> data, PortIndex portIndex)
{
  input = std::dynamic_pointer_cast<GxyData>(data);

  if (input)
  {
    output->di = input->di;
    input->di.print();
    input = std::dynamic_pointer_cast<GxyData>(data);
    data_range_min->setText(QString::number(input->di.data_min));
    data_range_max->setText(QString::number(input->di.data_max));
    _container->getApplyButton()->setEnabled(input && input->di.name != "");
  }
}

void 
VolumeVisModel::onDataRangeReset()
{
  data_range_min->setText(QString::number(input->di.data_min));
  data_range_max->setText(QString::number(input->di.data_max));
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

