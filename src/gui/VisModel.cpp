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

#include "VisModel.hpp"

VisModel::VisModel() 
{
  QFrame *frame  = new QFrame;
  QVBoxLayout *layout = new QVBoxLayout;
  layout->setSpacing(0);
  layout->setContentsMargins(2, 0, 2, 0);
  frame->setLayout(layout);

  QFrame *cmap_box = new QFrame;
  QHBoxLayout *cmap_box_layout = new QHBoxLayout;
  cmap_box->setLayout(cmap_box_layout);

  cmap_box_layout->addWidget(new QLabel("color map"));

  cmap_widget = new QLineEdit();
  cmap_box_layout->addWidget(cmap_widget);
  
  QPushButton *cmap_browse_button = new QPushButton("...");
  cmap_box_layout->addWidget(cmap_browse_button);
  connect(cmap_browse_button, SIGNAL(released()), this, SLOT(openCmapSelectorDialog()));

  layout->addWidget(cmap_box);

  QFrame *cmap_range_w = new QFrame;
  QHBoxLayout *cmap_range_l = new QHBoxLayout();
  cmap_range_l->setSpacing(0);
  cmap_range_l->setContentsMargins(2, 0, 2, 0);
  cmap_range_w->setLayout(cmap_range_l);

  cmap_range_l->addWidget(new QLabel("data range"));
  
  cmap_range_min = new QLineEdit;
  cmap_range_min->setText("0");
  cmap_range_min->setValidator(new QDoubleValidator());
  cmap_range_l->addWidget(cmap_range_min);
  
  cmap_range_max = new QLineEdit;
  cmap_range_max->setText("0");
  cmap_range_max->setValidator(new QDoubleValidator());
  cmap_range_l->addWidget(cmap_range_max);

  QPushButton *resetDataRange = new QPushButton("reset");
  connect(resetDataRange, SIGNAL(released()), this, SLOT(onDataRangeReset()));
  cmap_range_l->addWidget(resetDataRange);

  layout->addWidget(cmap_range_w);

  _properties->addProperties(frame);

  connect(cmap_range_max, SIGNAL(editingFinished()), this, SLOT(enableIfValid()));
  connect(cmap_range_min, SIGNAL(editingFinished()), this, SLOT(enableIfValid()));
  connect(_properties->getApplyButton(), SIGNAL(released()), this, SLOT(onApply()));

  enableIfValid();
}

void 
VisModel::onApply()
{
  std::cerr << "VisModel onApply should never be called (virtual)\n";
}

unsigned int
VisModel::nPorts(PortType portType) const
{
  return 1; // PortType::In or ::Out
}

void 
VisModel::onDataRangeReset()
{
  if (input)
  {
    cmap_range_min->setText(QString::number(input->dataInfo.data_min));
    cmap_range_max->setText(QString::number(input->dataInfo.data_max));
  }
}

QtNodes::NodeValidationState
VisModel::validationState() const
{
  return NodeValidationState::Valid;
}

QString
VisModel::validationMessage() const
{
  return QString("copacetic");
}

QJsonObject
VisModel::save() const 
{
  QJsonObject modelJson = GxyModel::save();
  output->toJson(modelJson);
  return modelJson;
}

bool 
VisModel::isValid() 
{
  bool valid = true;

  return (GxyModel::isValid() &&
          input && input->isValid() &&
          (cmap_widget->text().toStdString() != ""));
}

void
VisModel::loadInputDrivenWidgets(std::shared_ptr<GxyPacket> p) const
{
  if (p)
  {
    GxyModel::loadInputDrivenWidgets(p);
    std::shared_ptr<GxyData> d = std::dynamic_pointer_cast<GxyData>(p);
    cmap_range_min->setText(QString::number(d->dataInfo.data_min));
    cmap_range_max->setText(QString::number(d->dataInfo.data_max));
  }
}

void
VisModel::loadParameterWidgets() const
{
  if (output)
  {
    GxyModel::loadParameterWidgets();

    std::shared_ptr<Vis> v = std::dynamic_pointer_cast<Vis>(output);

    cmap_widget->setText(v->colormap_file.c_str());
    cmap_range_min->setText(QString::number(v->cmap_range_min));
    cmap_range_max->setText(QString::number(v->cmap_range_max));
  }
}

void
VisModel::loadOutput(std::shared_ptr<GxyPacket> p) const
{
  GxyModel::loadOutput(p);

  std::shared_ptr<Vis> v = std::dynamic_pointer_cast<Vis>(p);

  if (input)
    v->key = input->dataInfo.key;
  else
    v->key = -1;

  v->colormap_file = cmap_widget->text().toStdString();
  v->cmap_range_min = cmap_range_min->text().toDouble();
  v->cmap_range_max = cmap_range_max->text().toDouble();

  return;
}
