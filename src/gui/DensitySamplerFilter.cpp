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

#include "DensitySamplerFilter.hpp"

#include <QtGui/QIntValidator>
#include <QtGui/QDoubleValidator>

#include "GxyMainWindow.hpp"

DensitySamplerFilter::DensitySamplerFilter() 
{
  output = std::make_shared<GxyData>(getModelIdentifier());

  QFrame *frame  = new QFrame();
  QGridLayout *layout = new QGridLayout();
  layout->setSpacing(0);
  layout->setContentsMargins(2, 0, 2, 0);
  frame->setLayout(layout);

  layout->addWidget(new QLabel("Number of samples"), 0, 0);

  d_nSamples = new QLineEdit();
  d_nSamples->setText(QString::number(1000));
  d_nSamples->setValidator(new QIntValidator());
  layout->addWidget(d_nSamples, 0, 1);

  layout->addWidget(new QLabel("Raise to power"), 1, 0);

  d_power = new QLineEdit();
  d_power->setText(QString::number(1));
  d_power->setValidator(new QDoubleValidator());
  layout->addWidget(d_power, 1, 1);

  _properties->addProperties(frame);
}

unsigned int
DensitySamplerFilter::nPorts(QtNodes::PortType portType) const
{
  return 1;
}

QtNodes::NodeDataType
DensitySamplerFilter::dataType(QtNodes::PortType pt, QtNodes::PortIndex pi) const
{
  return GxyData().type();
}

void
DensitySamplerFilter::onApply()
{
  QJsonObject json = save();
  json["cmd"] = "gui::dsample";

  json["sourceKey"] = input->dataInfo.key;

  QJsonDocument doc(json);
  QByteArray bytes = doc.toJson(QJsonDocument::Compact);
  QString s = QLatin1String(bytes);

  std::string msg = s.toStdString();
  std::cerr << "DensitySamplerFilter request: " << msg << "\n";
  getTheGxyConnectionMgr()->CSendRecv(msg);
  std::cerr << "DensitySamplerFilter reply: " << msg << "\n";

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
DensitySamplerFilter::outData(QtNodes::PortIndex)
{
  std::cerr << "DensitySamplerFilter::outData ===============\n"; output->print();
  return std::static_pointer_cast<QtNodes::NodeData>(output);
}

QtNodes::NodeValidationState
DensitySamplerFilter::validationState() const
{
  return QtNodes::NodeValidationState::Valid;
}


QString
DensitySamplerFilter::validationMessage() const
{
  return QString("copacetic");
}

void
DensitySamplerFilter::setInData(std::shared_ptr<QtNodes::NodeData> data, QtNodes::PortIndex portIndex)
{
  input = std::dynamic_pointer_cast<GxyData>(data);
  
  if (input)
    loadInputDrivenWidgets(std::dynamic_pointer_cast<GxyPacket>(input));

  GxyFilter::setInData(data, portIndex);
  enableIfValid();

  if (isValid())
    Q_EMIT dataUpdated(0);
}

bool
DensitySamplerFilter::isValid()
{
  bool r = input && input->isValid();
  std::cerr << "DensitySamplerFilter::isValid :: " << r << "\n";
  return r;
}


QJsonObject
DensitySamplerFilter::save() const
{
  QJsonObject modelJson = GxyFilter::save();

  modelJson["nSamples"] = d_nSamples->text().toInt();
  modelJson["power"] = d_power->text().toDouble();

  return modelJson;
}

void
DensitySamplerFilter::restore(QJsonObject const &p)
{
  d_nSamples->setText(QString::number(p["nSamples"].toInt()));
  d_power->setText(QString::number(p["power"].toDouble()));

  loadParameterWidgets();
}

