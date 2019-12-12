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

#include "GxyMainWindow.hpp"
#include "StreamTracerModel.hpp"
#include "GxyValue.hpp"

StreamTracerModel::StreamTracerModel() 
{
  output = std::make_shared<GxyData>(getModelIdentifier());

  QFrame *frame  = new QFrame();
  QGridLayout *layout = new QGridLayout();
  frame->setLayout(layout);

  int row = 0;
  layout->addWidget(new QLabel("max steps"), row, 0);

  maxsteps = new QLineEdit();
  maxsteps->setText(QString::number(2000));
  maxsteps->setValidator(new QIntValidator());
  connect(maxsteps, &QLineEdit::editingFinished, [=] { retrace = true; });
  layout->addWidget(maxsteps, row++, 1);

  layout->addWidget(new QLabel("step size"), row, 0);

  stepsize = new QLineEdit();
  stepsize->setText(QString::number(0.2)); 
  stepsize->setValidator(new QDoubleValidator());
  connect(stepsize, &QLineEdit::editingFinished, [=] { retrace = true; });
  layout->addWidget(stepsize, row++, 1);

  layout->addWidget(new QLabel("min velocity"), row, 0);

  minvelocity = new QLineEdit();
  minvelocity->setText(QString::number(1e-12));
  minvelocity->setValidator(new QDoubleValidator());
  connect(minvelocity, &QLineEdit::editingFinished, [=] { retrace = true; });
  layout->addWidget(minvelocity, row++, 1);

  layout->addWidget(new QLabel("max time"), row, 0);

  maxtime = new QLineEdit();
  maxtime->setText(QString::number(-1.0));
  maxtime->setValidator(new QDoubleValidator());
  connect(maxtime, &QLineEdit::editingFinished, [=] { retrace = true; });
  layout->addWidget(maxtime, row++, 1);

  layout->addWidget(new QLabel("trim start"), row, 0);

  trimtime = new QLineEdit();
  trimtime->setText(QString::number(-1.0));
  trimtime->setValidator(new QDoubleValidator());
  connect(trimtime, &QLineEdit::editingFinished, [=] { retrim = true; });
  layout->addWidget(trimtime, row++, 1);

  layout->addWidget(new QLabel("trim delta time"), row, 0);

  trimdeltatime = new QLineEdit();
  trimdeltatime->setText(QString::number(-1.0));
  trimdeltatime->setValidator(new QDoubleValidator());
  connect(trimdeltatime, &QLineEdit::editingFinished, [=] { retrim = true; });
  layout->addWidget(trimdeltatime, row++, 1);

  _properties->addProperties(frame);
}

unsigned int
StreamTracerModel::nPorts(QtNodes::PortType portType) const
{
  if (portType == QtNodes::PortType::In)
    return 4;
  else
    return 1;
}

QtNodes::NodeDataType
StreamTracerModel::dataType(QtNodes::PortType pt, QtNodes::PortIndex pi) const
{
  if (pt == QtNodes::PortType::In)
  {
    if (pi == 0 || pi == 1)
      return GxyData().type();
    else if (pi == 2 || pi == 3)
      return GxyDouble().type();
  }

  return GxyData().type();
}

std::shared_ptr<QtNodes::NodeData>
StreamTracerModel::outData(QtNodes::PortIndex pi)
{
  return std::static_pointer_cast<QtNodes::NodeData>(output);
}

void
StreamTracerModel::
setInData(std::shared_ptr<QtNodes::NodeData> data, QtNodes::PortIndex pi)
{
  if (pi == 0)
  {
    std::shared_ptr<GxyData> d = std::dynamic_pointer_cast<GxyData>(data);
    if (d && d->isValid() && (! vectorField || vectorField->dataInfo.key != d->dataInfo.key))
    {
      vectorField = std::dynamic_pointer_cast<GxyData>(data);
      std::cerr << "vectorField is set to " << vectorField->dataInfo.key << "\n";
      retrace = true;
      retrim = true;
    }
  }

  else if (pi == 1)
  {
    std::shared_ptr<GxyData> d = std::dynamic_pointer_cast<GxyData>(data);
    if (d && d->isValid() && (! seeds || seeds->dataInfo.key != d->dataInfo.key))
    {
      seeds = std::dynamic_pointer_cast<GxyData>(data);
      std::cerr << "setting SEEDS " << seeds->dataInfo.key << "\n";
      retrace = true;
      retrim = true;
    }
  }

  else if (pi == 2)
  {
    std::shared_ptr<GxyDouble> d = std::dynamic_pointer_cast<GxyDouble>(data);
    if (trimtime->text().toDouble() != d->getValue())
    {
      trimtime->setText(QString::number(d->getValue()));
      retrim = true;
    }
  }

  else if (pi == 2)
  {
    std::shared_ptr<GxyDouble> d = std::dynamic_pointer_cast<GxyDouble>(data);
    if (trimdeltatime->text().toDouble() != d->getValue())
    {
      trimdeltatime->setText(QString::number(d->getValue()));
      retrim = true;
    }
  }
  else
    seeds = std::dynamic_pointer_cast<GxyData>(data);

  enableIfValid();
}


QtNodes::NodeValidationState
StreamTracerModel::validationState() const
{
  return QtNodes::NodeValidationState::Valid;
}


QString
StreamTracerModel::validationMessage() const
{
  return QString("copacetic");
}

QJsonObject
StreamTracerModel::save() const
{
  QJsonObject modelJson = GxyModel::save();

  modelJson["maxsteps"] = maxsteps->text().toInt();
  modelJson["stepsize"] = stepsize->text().toDouble();
  modelJson["minvelocity"] = minvelocity->text().toDouble();
  modelJson["maxtime"] = maxtime->text().toDouble();
  modelJson["trimtime"] = trimtime->text().toDouble();
  modelJson["trimdeltatime"] = trimdeltatime->text().toDouble();

  return modelJson;
}

void
StreamTracerModel::restore(QJsonObject const &p)
{
  maxsteps->setText(QString::number(p["maxsteps"].toInt()));
  stepsize->setText(QString::number(p["stepsize"].toDouble()));
  minvelocity->setText(QString::number(p["minvelocity"].toDouble()));
  maxtime->setText(QString::number(p["maxtime"].toDouble()));
  trimtime->setText(QString::number(p["trimtime"].toDouble()));
  trimdeltatime->setText(QString::number(p["trimdeltatime"].toDouble()));
}

void 
StreamTracerModel::onApply()
{
  QJsonObject json = save();

  json["vectorFieldKey"] = vectorField->dataInfo.key;
  json["seedsKey"] = seeds->dataInfo.key;

  if (retrace)
  {
    json["cmd"] = "gui::streamtracer";

    QJsonDocument doc(json);
    QByteArray bytes = doc.toJson(QJsonDocument::Compact);
    QString s = QLatin1String(bytes);
    std::string msg = s.toStdString();

    getTheGxyConnectionMgr()->CSendRecv(msg);

    rapidjson::Document rply;
    rply.Parse(msg.c_str());

    QString status = rply["status"].GetString();
    if (status.toStdString() != "ok")
    {
      std::cerr << "stream trace failed: " << rply["error message"].GetString() << "\n";
      return;
    }
    else
      std::cerr << "stream trace succeeded\n";
      

    retrace = false;
  }

  if (retrim)
  {
    std::cerr << "gui::trace2pathlines\n";
    json["cmd"] = "gui::trace2pathlines";

    QJsonDocument doc(json);
    QByteArray bytes = doc.toJson(QJsonDocument::Compact);
    QString s = QLatin1String(bytes);
    std::string msg = s.toStdString();

    getTheGxyConnectionMgr()->CSendRecv(msg);
    std::cerr << "gui::trace2pathlines reply: " << msg << "\n";

    rapidjson::Document rply;
    rply.Parse(msg.c_str());

    QString status = rply["status"].GetString();
    if (status.toStdString() != "ok")
    {
      std::cerr << "stream trim failed: " << rply["error message"].GetString() << "\n";
      return;
    }

    output->dataInfo.name = rply["name"].GetString();
    output->dataInfo.key = rply["key"].GetInt();
    output->dataInfo.type = rply["type"].GetInt();
    output->dataInfo.isVector = rply["isVector"].GetBool();
    output->dataInfo.data_min = rply["min"].GetDouble();
    output->dataInfo.data_max = rply["max"].GetDouble();
    for (auto i = 0; i < 6; i++)
      output->dataInfo.box[i] = rply["box"][i].GetDouble();

    retrim = true;
    output->setValid(true);

    Q_EMIT dataUpdated(0);
  }
}

bool 
StreamTracerModel::isValid()
{
  return vectorField && vectorField->isValid() && seeds && seeds->isValid();
}
