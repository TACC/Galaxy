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

#include "RenderModel.hpp"

#include <QJsonDocument>
#include <QtGui/QDoubleValidator>

RenderModel::RenderModel() 
{
  RenderModel::init();

  QFrame *frame  = new QFrame();
  QVBoxLayout *layout = new QVBoxLayout();
  layout->setSpacing(0);
  layout->setContentsMargins(2, 0, 2, 0);
  frame->setLayout(layout);

  QPushButton *openCamera = new QPushButton("Camera");
  connect(openCamera, SIGNAL(released()), this, SLOT(openCameraDialog()));
  layout->addWidget(openCamera);

  QPushButton *openLights = new QPushButton("Lights");
  connect(openLights, SIGNAL(released()), this, SLOT(openLightsDialog()));
  layout->addWidget(openLights);
  
  QWidget *update_widget = new QWidget;
  QHBoxLayout *update_layout = new QHBoxLayout;
  update_widget->setLayout(update_layout);
  QLabel *update_label = new QLabel("Update rate:");
  update_layout->addWidget(update_label);
  update_rate = new QLineEdit;
  update_rate->setValidator(new QDoubleValidator());
  update_rate->setText(QString::number(update_rate_msec == 0.0 ? 0.0 : 1000.0 / update_rate_msec));
  update_layout->addWidget(update_rate);
  layout->addWidget(update_widget);
  connect(update_rate, SIGNAL(editingFinished()), this, SLOT(setUpdateRate()));

  timer = new QTimer(this);
  timer->setSingleShot(true);
  timer->setInterval(update_rate_msec);
  connect(timer, SIGNAL(timeout()), this, SLOT(timeout()));

  _container->setCentralWidget(frame);

  connect(this, SIGNAL(cameraChanged(Camera&)), &renderWindow, SLOT(onCameraChanged(Camera&)));
  renderWindow.show();

  QPushButton *open = new QPushButton("Open");
  connect(open, SIGNAL(released()), &renderWindow, SLOT(show()));
  _container->addButton(open);

  connect(_container->getApplyButton(), SIGNAL(released()), this, SLOT(onApply()));

  connect(getTheGxyConnectionMgr(), SIGNAL(connectionStateChanged(bool)), this, SLOT(onConnectionStateChanged(bool)));

  if (getTheGxyConnectionMgr()->IsConnected())
    renderWindow.manageThreads(true);

  Q_EMIT(cameraChanged(camera));
}

unsigned int
RenderModel::nPorts(PortType portType) const
{
  if (portType == PortType::In)
    return 1;
  else
    return 0;
}

NodeDataType
RenderModel::dataType(PortType pt, PortIndex pi) const
{
  return GxyVis().type();
}

std::shared_ptr<NodeData>
RenderModel::outData(PortIndex)
{
  return NULL;
}

void
RenderModel::
setInData(std::shared_ptr<NodeData> data, PortIndex portIndex)
{
  input = std::dynamic_pointer_cast<GxyVis>(data);
  if (input) visList[input->get_origin()] = input;
  _container->getApplyButton()->setEnabled(visList.size() > 0);

  std::cerr << "RenderModel " << getModelIdentifier() << " receives:\n";
  if (input) input->print();
  else std::cerr << "nothing\n";

#if 0
  if (data)
  {
    for (auto vis : visList)
      vis.second->print();
  }
#endif

  if (portIndex == 0)
  {
    std::shared_ptr<GxyVis> vis = std::dynamic_pointer_cast<GxyVis>(data);
    Q_EMIT visUpdated(vis);
  }
}

NodeValidationState
RenderModel::validationState() const
{
  return NodeValidationState::Valid;
}

QString
RenderModel::validationMessage() const
{
  return QString("copacetic");
}

QJsonObject
RenderModel::save() const
{
  QJsonObject modelJson = NodeDataModel::save();

  modelJson["camera"] = camera.save();
  modelJson["lighting"] = lighting.save();

  return modelJson;
}

void
RenderModel::restore(QJsonObject const &p)
{
  camera.restore(p["camera"].toObject());
  lighting.restore(p["lighting"].toObject());
}

void
RenderModel::setUpdateRate()
{
  std::cerr << "update rate set to " << update_rate->text().toDouble() << "\n";

  float ups = update_rate->text().toDouble();
  if (ups > 0.0)
  {
    update_rate_msec = 1000.0 / ups;
    timer->setInterval(update_rate_msec);
    timer->start();
  }
  else
    update_rate_msec = 0;
}

void 
RenderModel::timeout()
{
  if (update_rate_msec > 0)
  {
    renderWindow.Update();
    timer->start();
  }
}

void
RenderModel::sendCamera()
{
  QJsonObject cameraJson;
  cameraJson["Camera"] = camera.save();
  QJsonDocument doc(cameraJson);
  QByteArray bytes = doc.toJson(QJsonDocument::Compact);
  QString qs = QLatin1String(bytes);
  std::string msg = std::string("json ") + qs.toStdString();
  std::cerr << "Camera: " << msg << "\n";
  getTheGxyConnectionMgr()->CSendRecv(msg);

  gxy::vec2i window_size = camera.getSize();
  std::stringstream ss;
  ss << "window " << window_size.x << " " << window_size.y;
  std::string s = ss.str();
  getTheGxyConnectionMgr()->CSendRecv(s);
}

void
RenderModel::sendVisualization()
{
  QJsonObject visualizationJson;

  QJsonObject v;
  v["Lighting"] = lighting.save();

  QJsonArray operatorsJson;
  for (auto vis : visList)
  {
    QJsonObject operatorJson;
    vis.second->save(operatorJson);
    operatorsJson.push_back(operatorJson);
  }
  v["operators"] = operatorsJson;

  visualizationJson["Visualization"] = v;

  QJsonDocument doc(visualizationJson);
  QByteArray bytes = doc.toJson(QJsonDocument::Compact);
  QString s = QLatin1String(bytes);

  std::string msg = std::string("json ") + s.toStdString();
  std::cerr << "Visualization: " << msg << "\n";
  getTheGxyConnectionMgr()->CSendRecv(msg);
}

void
RenderModel::render()
{
  std::string s("render");
  getTheGxyConnectionMgr()->CSendRecv(s);
  std::cerr << "render reply: " << s << "\n";
}

