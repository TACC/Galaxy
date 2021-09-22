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

#include "MHSamplerFilter.hpp"

#include <QtGui/QIntValidator>
#include <QtGui/QDoubleValidator>

#include "GxyMainWindow.hpp"

MHSamplerFilter::MHSamplerFilter() 
{
  output = std::make_shared<GxyData>(getModelIdentifier());

  QFrame *frame  = new QFrame();
  QVBoxLayout *layout = new QVBoxLayout();
  layout->setSpacing(0);
  layout->setContentsMargins(2, 0, 2, 0);
  frame->setLayout(layout);

  mh_properties = new QFrame;
  QGridLayout *mhl = new QGridLayout;
  mh_properties->setLayout(mhl);

  QWidget *tfunc_widget = new QWidget;
  QVBoxLayout *l1 = new QVBoxLayout;
  tfunc_widget->setLayout(l1);

  QWidget *w = new QWidget;
  QHBoxLayout *l2 = new QHBoxLayout;
  w->setLayout(l2);

  l2->addWidget(new QLabel("transfer function"));
  mh_tfunc = new QComboBox();
  mh_tfunc->addItem("none");
  mh_tfunc->addItem("linear");
  mh_tfunc->addItem("gaussian");
  connect(mh_tfunc, SIGNAL(currentIndexChanged(int)), this, SLOT(mh_tfunc_Changed(int)));
  l2->addWidget(mh_tfunc);
  l1->addWidget(w);

  linear = new QFrame;
  QHBoxLayout *mml = new QHBoxLayout;
  linear->setLayout(mml);
  mml->addWidget(new QLabel("min"));
  mh_linear_tf_min = new QLineEdit;
  mh_linear_tf_min->setValidator(new QDoubleValidator());
  mml->addWidget(mh_linear_tf_min);
  mml->addWidget(new QLabel("max"));
  mh_linear_tf_max = new QLineEdit;
  mh_linear_tf_max->setValidator(new QDoubleValidator());
  mml->addWidget(mh_linear_tf_max);
  linear->setVisible(false);
  l1->addWidget(linear);

  gaussian = new QFrame;
  QHBoxLayout *gl = new QHBoxLayout;
  gaussian->setLayout(gl);
  gl->addWidget(new QLabel("mean"));
  gaussian_mean = new QLineEdit;
  gaussian_mean->setText(QString::number(0));
  gl->addWidget(gaussian_mean);
  gl->addWidget(new QLabel("std"));
  gaussian_std = new QLineEdit;
  gaussian_std->setText(QString::number(1));
  gl->addWidget(gaussian_std);
  gaussian->setVisible(false);
  l1->addWidget(gaussian);

  mhl->addWidget(tfunc_widget, 0, 0, 1, 2);

  mhl->addWidget(new QLabel("radius"), 2, 0);
  mh_radius = new QLineEdit();
  mh_radius->setText(QString::number(0.02));
  mh_radius->setValidator(new QDoubleValidator());
  mhl->addWidget(mh_radius, 2, 1);

  mhl->addWidget(new QLabel("sigma"), 3, 0);
  mh_sigma = new QLineEdit();
  mh_sigma->setText(QString::number(1.0));
  mh_sigma->setValidator(new QDoubleValidator());
  mhl->addWidget(mh_sigma, 3, 1);

  mhl->addWidget(new QLabel("iterations"), 4, 0);
  mh_iterations = new QLineEdit();
  mh_iterations->setValidator(new QIntValidator());
  mh_iterations->setText(QString::number(20000));
  mhl->addWidget(mh_iterations, 4, 1);

  mhl->addWidget(new QLabel("startup"), 5, 0);
  mh_startup = new QLineEdit();
  mh_startup->setValidator(new QIntValidator());
  mh_startup->setText(QString::number(1000));
  mhl->addWidget(mh_startup, 5, 1);

  mhl->addWidget(new QLabel("skip"), 6, 0);
  mh_skip = new QLineEdit();
  mh_skip->setValidator(new QIntValidator());
  mh_skip->setText(QString::number(10));
  mhl->addWidget(mh_skip, 6, 1);

  mhl->addWidget(new QLabel("miss"), 7, 0);
  mh_miss = new QLineEdit();
  mh_miss->setValidator(new QIntValidator());
  mh_miss->setText(QString::number(10));
  mhl->addWidget(mh_miss, 7, 1);

  layout->addWidget(mh_properties);

  _properties->addProperties(frame);
}

unsigned int
MHSamplerFilter::nPorts(QtNodes::PortType portType) const
{
  if (portType == QtNodes::PortType::In)
    return 1;
  else
    return 1;
}

QtNodes::NodeDataType
MHSamplerFilter::dataType(QtNodes::PortType pt, QtNodes::PortIndex pi) const
{
  return GxyData().type();
}

void
MHSamplerFilter::onApply()
{
  QJsonObject json = save();
  json["cmd"] = "gui::mhsample";

  json["source"] = input->dataInfo.name.c_str();

  QJsonDocument doc(json);
  QByteArray bytes = doc.toJson(QJsonDocument::Compact);
  QString s = QLatin1String(bytes);

  std::string msg = s.toStdString();
  std::cerr << "MHSamplerFilter request: " << msg << "\n";
  getTheGxyConnectionMgr()->CSendRecv(msg);
  std::cerr << "MHSamplerFilter reply: " << msg << "\n";

  rapidjson::Document dset;
  dset.Parse(msg.c_str());

  output->dataInfo.name = dset["name"].GetString();
  output->dataInfo.type = dset["type"].GetInt();
  output->dataInfo.isVector = (dset["ncomp"].GetInt() == 3);
  output->dataInfo.data_min = dset["min"].GetDouble();
  output->dataInfo.data_max = dset["max"].GetDouble();

  output->setValid(true);

  GxyFilter::onApply();
}

QtNodes::NodeValidationState
MHSamplerFilter::validationState() const
{
  return QtNodes::NodeValidationState::Valid;
}


QString
MHSamplerFilter::validationMessage() const
{
  return QString("copacetic");
}

void
MHSamplerFilter::setInData(std::shared_ptr<QtNodes::NodeData> data, QtNodes::PortIndex portIndex)
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
MHSamplerFilter::loadInputDrivenWidgets(std::shared_ptr<GxyPacket> o) 
{
  std::shared_ptr<GxyData> input = std::dynamic_pointer_cast<GxyData>(o);
  mh_linear_tf_min->setText(QString::number(input->dataInfo.data_min));
  mh_linear_tf_max->setText(QString::number(input->dataInfo.data_max));
}

bool
MHSamplerFilter::isValid()
{
  bool r = input && input->isValid();
  std::cerr << "MHSamplerFilter::isValid :: " << r << "\n";
  return r;
}


QJsonObject
MHSamplerFilter::save() const
{
  QJsonObject modelJson = GxyFilter::save();

  modelJson["mh_tfunc"] = mh_tfunc->currentIndex();

  modelJson["mh_radius"] = mh_radius->text().toDouble();
  modelJson["mh_sigma"] = mh_sigma->text().toDouble();
  modelJson["mh_iterations"] = mh_iterations->text().toInt();
  modelJson["mh_startup"] = mh_startup->text().toInt();
  modelJson["mh_skip"] = mh_skip->text().toInt();
  modelJson["mh_miss"] = mh_miss->text().toInt();
  modelJson["mh_linear_tf_min"] = mh_linear_tf_min->text().toDouble();
  modelJson["mh_linear_tf_max"] = mh_linear_tf_max->text().toDouble();
  modelJson["gaussian_mean"] = gaussian_mean->text().toDouble();
  modelJson["gaussian_std"] = gaussian_std->text().toDouble();

  return modelJson;
}

void
MHSamplerFilter::restore(QJsonObject const &p)
{
  mh_tfunc->setCurrentIndex(p["mh_tfunc"].toInt());
  mh_radius->setText(QString::number(p["mh_radius"].toDouble()));
  mh_sigma->setText(QString::number(p["mh_sigma"].toDouble()));
  mh_iterations->setText(QString::number(p["mh_iterations"].toInt()));
  mh_startup->setText(QString::number(p["mh_startup"].toInt()));
  mh_skip->setText(QString::number(p["mh_skip"].toInt()));
  mh_miss->setText(QString::number(p["mh_miss"].toInt()));
  mh_linear_tf_min->setText(QString::number(p["mh_linear_tf_min"].toDouble()));
  mh_linear_tf_max->setText(QString::number(p["mh_linear_tf_max"].toDouble()));
  gaussian_mean->setText(QString::number(p["gaussian_mean"].toDouble()));
  gaussian_std->setText(QString::number(p["gaussian_std"].toDouble()));

  loadParameterWidgets();
}

