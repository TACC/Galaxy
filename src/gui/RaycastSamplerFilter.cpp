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

#include "RaycastSamplerFilter.hpp"

#include <QtGui/QIntValidator>
#include <QtGui/QDoubleValidator>

#include "GxyMainWindow.hpp"

RaycastSamplerFilter::RaycastSamplerFilter() 
{
  output = std::make_shared<GxyData>(getModelIdentifier());

  QFrame *frame  = new QFrame();
  QVBoxLayout *layout = new QVBoxLayout();
  layout->setSpacing(2);
  layout->setContentsMargins(2, 2, 2, 2);
  frame->setLayout(layout);

  QWidget *algorithm = new QWidget();
  QGridLayout *algorithm_layout = new QGridLayout();
  algorithm->setLayout(algorithm_layout);
  algorithm_layout->addWidget(new QLabel("algorithm"), 0, 0);
  
  type = new QComboBox();
  type->addItem("raycast-isosurface");
  type->addItem("raycast-gradient");
  connect(type, SIGNAL(currentIndexChanged(int)), this, SLOT(algorithmChanged(int)));

  algorithm_layout->addWidget(type, 0, 1);
  layout->addWidget(algorithm);

  raycast_properties = new QFrame;
  raycast_properties->setVisible(true);
  QVBoxLayout *rcl = new QVBoxLayout;
  rcl->setSpacing(2);
  rcl->setContentsMargins(2, 2, 2, 2);
  raycast_properties->setLayout(rcl);

  openCamera = new QPushButton("Camera");
  connect(openCamera, SIGNAL(released()), this, SLOT(openCameraDialog()));
  rcl->addWidget(openCamera);

  isovalue_properties = new QFrame;
  isovalue_properties->setVisible(true);
  QHBoxLayout *rl = new QHBoxLayout;
  rl->setSpacing(2);
  rl->setContentsMargins(2, 2, 2, 2);
  isovalue_properties->setLayout(rl);
  rl->addWidget(new QLabel("isovalue"));
  isovalue = new QLineEdit;
  isovalue->setValidator(new QDoubleValidator());
  connect(isovalue, SIGNAL(editingFinished()), this, SLOT(enableIfValid()));
  rl->addWidget(isovalue);

  rcl->addWidget(isovalue_properties);

  gradient_properties = new QFrame;
  gradient_properties->setVisible(false);
  rl = new QHBoxLayout;
  rl->setSpacing(2);
  rl->setContentsMargins(2, 2, 2, 2);
  gradient_properties->setLayout(rl);
  rl->addWidget(new QLabel("tolerance"));
  tolerance = new QLineEdit;
  tolerance->setValidator(new QDoubleValidator());
  connect(tolerance, SIGNAL(editingFinished()), this, SLOT(enableIfValid()));
  rl->addWidget(tolerance);

  rcl->addWidget(gradient_properties);

  layout->addWidget(raycast_properties);

  _properties->addProperties(frame);
}

unsigned int
RaycastSamplerFilter::nPorts(QtNodes::PortType portType) const
{
  if (portType == QtNodes::PortType::In)
    return 1;
  else
    return 1;
}

QtNodes::NodeDataType
RaycastSamplerFilter::dataType(QtNodes::PortType pt, QtNodes::PortIndex pi) const
{
  return GxyData().type();
}

void
RaycastSamplerFilter::onApply()
{
  QJsonObject json = save();
  json["cmd"] = "gui::raysample";

  json["sourceKey"] = input->dataInfo.key;

  QJsonDocument doc(json);
  QByteArray bytes = doc.toJson(QJsonDocument::Compact);
  QString s = QLatin1String(bytes);

  std::string msg = s.toStdString();
  getTheGxyConnectionMgr()->CSendRecv(msg);

  rapidjson::Document dset;
  dset.Parse(msg.c_str());

  output->dataInfo.name = dset["name"].GetString();
  output->dataInfo.key = dset["key"].GetInt();
  output->dataInfo.type = dset["type"].GetInt();
  output->dataInfo.isVector = (dset["ncomp"].GetInt() == 3);
  output->dataInfo.data_min = dset["min"].GetDouble();
  output->dataInfo.data_max = dset["max"].GetDouble();
  for (auto i = 0; i < 6; i++)
    output->dataInfo.box[i] = dset["box"][i].GetDouble();

  output->setValid(true);

  GxyFilter::onApply();
}

std::shared_ptr<QtNodes::NodeData>
RaycastSamplerFilter::outData(QtNodes::PortIndex)
{
  return std::static_pointer_cast<QtNodes::NodeData>(output);
}

QtNodes::NodeValidationState
RaycastSamplerFilter::validationState() const
{
  return QtNodes::NodeValidationState::Valid;
}


QString
RaycastSamplerFilter::validationMessage() const
{
  return QString("copacetic");
}

void
RaycastSamplerFilter::setInData(std::shared_ptr<QtNodes::NodeData> data, QtNodes::PortIndex portIndex)
{
  input = std::dynamic_pointer_cast<GxyData>(data);
  
  if (input)
    loadInputDrivenWidgets(std::dynamic_pointer_cast<GxyPacket>(input));

  GxyFilter::setInData(data, portIndex);

  enableIfValid();

  if (isValid())
    Q_EMIT dataUpdated(0);
}

void
RaycastSamplerFilter::loadInputDrivenWidgets(std::shared_ptr<GxyPacket> o) 
{
}

bool
RaycastSamplerFilter::isValid()
{
  if (! GxyFilter::isValid()) return false;
  if (! input) return false;
  if (! input->isValid()) return false;
  switch(type->currentIndex())
  {
    case 0: if (isovalue->text().toStdString() == "") return false;
            break;
    case 1: if (tolerance->text().toStdString() == "") return false;
            break;
    default: return false;
  }

  return true;
}


QJsonObject
RaycastSamplerFilter::save() const
{
  QJsonObject modelJson = GxyFilter::save();

  modelJson["camera"] = camera.save();

  modelJson["type"] = type->currentIndex();
  modelJson["isovalue"] = isovalue->text().toDouble();
  modelJson["tolerance"] = tolerance->text().toDouble();

  return modelJson;
}

void
RaycastSamplerFilter::restore(QJsonObject const &p)
{
  camera.restore(p["camera"].toObject());

  type->setCurrentIndex(p["type"].toInt());
  isovalue->setText(QString::number(p["isovalue"].toDouble()));
  tolerance->setText(QString::number(p["tolerance"].toDouble()));

  loadParameterWidgets();
}

