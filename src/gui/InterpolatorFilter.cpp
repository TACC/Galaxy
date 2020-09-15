// ========================================================================== //
//                                                                            //
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

#include "InterpolatorFilter.hpp"

InterpolatorFilter::InterpolatorFilter() 
{
  output = std::make_shared<GxyData>(getModelIdentifier());

  QFrame *frame  = new QFrame();
  QGridLayout *layout = new QGridLayout();
  frame->setLayout(layout);

  _properties->addProperties(frame);
  connect(_properties->getApplyButton(), SIGNAL(released()), this, SLOT(onApply()));
}

unsigned int
InterpolatorFilter::nPorts(QtNodes::PortType portType) const
{
  if (portType == QtNodes::PortType::In)
    return 2;
  else
    return 1;
}

QtNodes::NodeDataType
InterpolatorFilter::dataType(QtNodes::PortType pt, QtNodes::PortIndex) const
{
  return GxyData().type();
}

void
InterpolatorFilter::onApply()
{
  QJsonObject json = save();
  json["cmd"] = "gui::interpolate";

  json["volume"] = volume->dataInfo.name.c_str();
  json["pointset"] = pset->dataInfo.name.c_str();

  QJsonDocument doc(json);
  QByteArray bytes = doc.toJson(QJsonDocument::Compact);
  QString s = QLatin1String(bytes);

  std::string msg = s.toStdString();
  std::cerr << "InterpolatorFilter request: " << msg << "\n";
  getTheGxyConnectionMgr()->CSendRecv(msg);
  std::cerr << "InterpolatorFilter reply: " << msg << "\n";

  rapidjson::Document rply;
  rply.Parse(msg.c_str());

  QString status = rply["status"].GetString();
  if (status.toStdString() != "ok")
  {
    std::cerr << "interpolation failed: " << rply["error message"].GetString() << "\n";
    return;
  }


  output->dataInfo.name = rply["name"].GetString();
  output->dataInfo.type = rply["type"].GetInt();
  output->dataInfo.isVector = (rply["ncomp"].GetInt() == 3);
  output->dataInfo.data_min = rply["min"].GetDouble();
  output->dataInfo.data_max = rply["max"].GetDouble();
  for (auto i = 0; i < 6; i++)
    output->dataInfo.box[i] = rply["box"][i].GetDouble();

  output->setValid(true);

  GxyFilter::onApply();
}

void
InterpolatorFilter::
setInData(std::shared_ptr<QtNodes::NodeData> data, QtNodes::PortIndex portIndex)
{
  if (portIndex == 0)
    volume = std::dynamic_pointer_cast<GxyData>(data);
  else if (portIndex == 1)
    pset = std::dynamic_pointer_cast<GxyData>(data);
  else
    std::cerr << "more than 2 input ports?\n";

  GxyFilter::setInData(data, portIndex);

  if (isValid())
    Q_EMIT dataUpdated(0);
}

bool 
InterpolatorFilter::isValid()
{
  return volume && volume->isValid() && pset && pset->isValid();
}

QtNodes::NodeValidationState
InterpolatorFilter::validationState() const
{
  return QtNodes::NodeValidationState::Valid;
}


QString
InterpolatorFilter::validationMessage() const
{
  return QString("copacetic");
}

