// ========================================================================== //
// Copyright (c) 2014-2020 The University of Texas at Austin.                 //
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

#include "PathlinesVis.hpp"
#include "PathlinesVisModel.hpp"

PathlinesVisModel::PathlinesVisModel() 
{
  output = std::dynamic_pointer_cast<Vis>(std::shared_ptr<PathlinesVis>(new PathlinesVis(model_identifier)));
  output->setValid(false);

  QFrame *frame  = new QFrame();
  QVBoxLayout *layout = new QVBoxLayout();
  layout->setSpacing(0);
  layout->setContentsMargins(2, 0, 2, 0);
  frame->setLayout(layout);

  QWidget     *radius_box = new QWidget();
  QGridLayout *radius_box_layout = new QGridLayout();
  radius_box->setLayout(radius_box_layout);

  radius_box_layout->addWidget(new QLabel("data range"), 0, 0);

  minrange = new QLineEdit();
  minrange->setValidator(new QDoubleValidator());
  minrange->setText(QString::number(0.0));
  radius_box_layout->addWidget(minrange, 0, 2);

  maxrange = new QLineEdit();
  maxrange->setValidator(new QDoubleValidator());
  maxrange->setText(QString::number(1.0));
  radius_box_layout->addWidget(maxrange, 0, 3);

  radius_box_layout->addWidget(new QLabel("radius range"), 1, 0);

  minradius = new QLineEdit();
  minradius->setValidator(new QDoubleValidator());
  minradius->setText(QString::number(0.0));
  radius_box_layout->addWidget(minradius, 1, 2);

  maxradius = new QLineEdit();
  maxradius->setValidator(new QDoubleValidator());
  maxradius->setText(QString::number(1.0));
  radius_box_layout->addWidget(maxradius, 1, 3);

  layout->addWidget(radius_box);

  _properties->addProperties(frame);
}

unsigned int
PathlinesVisModel::nPorts(QtNodes::PortType portType) const
{
  return 1; // QtNodes::PortType::In or ::Out
}

QtNodes::NodeDataType
PathlinesVisModel::dataType(QtNodes::PortType pt, QtNodes::PortIndex) const
{
  if (pt == QtNodes::PortType::In)
    return GxyData().type();
  else
    return Vis().type();
}

void
PathlinesVisModel::
setInData(std::shared_ptr<QtNodes::NodeData> data, QtNodes::PortIndex portIndex)
{
  VisModel::setInData(data, portIndex);
  
  if (input)
    loadInputDrivenWidgets(std::dynamic_pointer_cast<GxyPacket>(input));

  if (isValid())
  {
    enable(true);
    onApply();
  }
  else
    enable(false);
}

void
PathlinesVisModel::loadInputDrivenWidgets(std::shared_ptr<GxyPacket> o) 
{
  if (input)
  {
    VisModel::loadInputDrivenWidgets(input);

    std::shared_ptr<GxyData> d = std::dynamic_pointer_cast<GxyData>(input);

    minrange->setText(QString::number(d->dataInfo.data_min));
    maxrange->setText(QString::number(d->dataInfo.data_max));
  }
}


QtNodes::NodeValidationState
PathlinesVisModel::validationState() const
{
  return QtNodes::NodeValidationState::Valid;
}


QString
PathlinesVisModel::validationMessage() const
{
  return QString("copacetic");
}

QJsonObject
PathlinesVisModel::save() const
{
  loadOutput(output);

  QJsonObject modelJson = VisModel::save();

  loadOutput(std::dynamic_pointer_cast<GxyData>(output));
  output->toJson(modelJson);

  return modelJson;
}

void
PathlinesVisModel::restore(QJsonObject const &p)
{
  output->fromJson(p);
  loadParameterWidgets();
}

bool
PathlinesVisModel::isValid()
{
  // if (! input) std::cerr << "PLVM input null\n";
  // if (input && ! input->isValid())  std::cerr << "PLVM input not valid\n";
  // if (! VisModel::isValid()) std::cerr << "PLVM VisModel not valid\n";
  return (input && input->isValid() && VisModel::isValid());
}

void
PathlinesVisModel::loadOutput(std::shared_ptr<GxyData> o) const
{
  VisModel::loadOutput(o);

  std::shared_ptr<PathlinesVis> v = std::dynamic_pointer_cast<PathlinesVis>(o);

  v->minrange = minrange->text().toDouble();
  v->maxrange = maxrange->text().toDouble();
  v->minradius = minradius->text().toDouble();
  v->maxradius = maxradius->text().toDouble();
}

void
PathlinesVisModel::loadParameterWidgets() 
{
  VisModel::loadParameterWidgets();

  std::shared_ptr<PathlinesVis> v = std::dynamic_pointer_cast<PathlinesVis>(output);

  minrange->setText(QString::number(v->minrange));
  maxrange->setText(QString::number(v->maxrange));

  minradius->setText(QString::number(v->minradius));
  maxradius->setText(QString::number(v->maxradius));
}

void
PathlinesVisModel::onApply()
{
  std::cerr << "PLVM onApply\n";
  if (isValid())
  {
    std::cerr << "PLVM isValid is true\n";
    output = std::shared_ptr<PathlinesVis>(new PathlinesVis(model_identifier));
    loadOutput(std::dynamic_pointer_cast<GxyData>(output));
    output->setValid(true);

    VisModel::onApply();
  }
  else
    std::cerr << "PLVM isValid is false\n";
}


