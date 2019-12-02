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

  isovaluesDialog = new ScalarsDialog;
  slicesDialog = new PlanesDialog;

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

  QFrame *xfunc_box = new QFrame();
  QHBoxLayout *xfunc_box_layout = new QHBoxLayout();
  xfunc_box->setLayout(xfunc_box_layout);

  xfunc_box_layout->addWidget(new QLabel("transfer function"));

  xfunc_widget = new QLineEdit();
  xfunc_box_layout->addWidget(xfunc_widget);
  
  QPushButton *tfunc_browse_button = new QPushButton("...");
  xfunc_box_layout->addWidget(tfunc_browse_button);
  connect(tfunc_browse_button, SIGNAL(released()), this, SLOT(openFileSelectorDialog()));

  layout->addWidget(xfunc_box);

  QFrame *xfunc_range_w = new QFrame;
  QHBoxLayout *xfunc_range_l = new QHBoxLayout();
  xfunc_range_l->setSpacing(0);
  xfunc_range_l->setContentsMargins(2, 0, 2, 0);
  xfunc_range_w->setLayout(xfunc_range_l);

  xfunc_range_l->addWidget(new QLabel("data range"));
  
  xfunc_range_min = new QLineEdit;
  xfunc_range_min->setText("0");
  xfunc_range_min->setValidator(new QDoubleValidator());
  xfunc_range_l->addWidget(xfunc_range_min);
  
  xfunc_range_max = new QLineEdit;
  xfunc_range_max->setText("0");
  xfunc_range_max->setValidator(new QDoubleValidator());
  xfunc_range_l->addWidget(xfunc_range_max);

  QPushButton *resetDataRange = new QPushButton("reset");
  connect(resetDataRange, SIGNAL(released()), this, SLOT(onDataRangeReset()));
  xfunc_range_l->addWidget(resetDataRange);

  layout->addWidget(xfunc_range_w);

  _properties->addProperties(frame);

  connect(xfunc_range_max, SIGNAL(editingFinished()), this, SLOT(enableApply()));
  connect(xfunc_range_min, SIGNAL(editingFinished()), this, SLOT(enableApply()));
  connect(_properties->getApplyButton(), SIGNAL(released()), this, SLOT(onApply()));
}

VolumeVisModel::~VolumeVisModel()
{
  delete isovaluesDialog;
  delete slicesDialog;
}

void
VolumeVisModel::loadInputDrivenWidgets(std::shared_ptr<GxyPacket> o) const
{
  if (input)
  {
    VisModel::loadInputDrivenWidgets(input);

    std::shared_ptr<GxyData> d = std::dynamic_pointer_cast<GxyData>(input);

    xfunc_range_min->setText(QString::number(d->dataInfo.data_min));
    xfunc_range_max->setText(QString::number(d->dataInfo.data_min));
  }
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

  xfunc_widget->setText(v->transfer_function.c_str());
  xfunc_range_min->setText(QString::number(v->xfer_range_min));
  xfunc_range_max->setText(QString::number(v->xfer_range_max));
}

void
VolumeVisModel::loadOutput(std::shared_ptr<GxyPacket> o) const
{
  VisModel::loadOutput(o);

  std::shared_ptr<VolumeVis> v = std::dynamic_pointer_cast<VolumeVis>(o);

  v->slices = slicesDialog->get_planes();
  v->isovalues = isovaluesDialog->get_scalars();
  
  v->volume_rendering_flag = volumeRender->isChecked();
  v->transfer_function = xfunc_widget->text().toStdString();
  v->xfer_range_min = xfunc_range_min->text().toDouble();
  v->xfer_range_min = xfunc_range_min->text().toDouble();
}

void 
VolumeVisModel::onApply()
{
  output = std::shared_ptr<VolumeVis>(new VolumeVis(model_identifier));
  loadOutput(std::dynamic_pointer_cast<GxyPacket>(output));
  output->setValid(true);
  std::cerr << "onApply onApply onApply onApply onApply onApply onApply onApply \n";
  output->print();

  Q_EMIT dataUpdated(0);
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
  std::cerr << "+++++++++++++++++++ VolumeVisModel::setInData\n";
  input->print();
  
  if (input)
    loadInputDrivenWidgets(std::dynamic_pointer_cast<GxyPacket>(input));

  enableIfValid();
}

void 
VolumeVisModel::onDataRangeReset()
{
  xfunc_range_min->setText(QString::number(input->dataInfo.data_min));
  xfunc_range_max->setText(QString::number(input->dataInfo.data_max));
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

  if (volumeRender->isChecked() && xfunc_widget->text().toStdString() == "")
  {
    return false;
  }

  return true;
}
