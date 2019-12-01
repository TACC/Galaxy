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

#include "SamplerModel.hpp"

#include <QtGui/QIntValidator>
#include <QtGui/QDoubleValidator>

SamplerModel::SamplerModel() 
{
  QFrame *frame  = new QFrame();
  QVBoxLayout *layout = new QVBoxLayout();
  layout->setSpacing(0);
  layout->setContentsMargins(2, 0, 2, 0);
  frame->setLayout(layout);

  QWidget *algorithm = new QWidget();
  QGridLayout *algorithm_layout = new QGridLayout();
  algorithm->setLayout(algorithm_layout);
  algorithm_layout->addWidget(new QLabel("algorithm"), 0, 0);
  
  type = new QComboBox();
  type->addItem("metropolis-hastings");
  type->addItem("raycast-gradient");
  type->addItem("raycast-isosurface");
  connect(type, SIGNAL(currentIndexChanged(int)), this, SLOT(algorithmChanged(int)));

  algorithm_layout->addWidget(type, 0, 1);
  layout->addWidget(algorithm);

  raycast_properties = new QFrame;
  raycast_properties->setVisible(false);
  QVBoxLayout *rcl = new QVBoxLayout;
  raycast_properties->setLayout(rcl);

  openCamera = new QPushButton("Camera");
  connect(openCamera, SIGNAL(released()), this, SLOT(openCameraDialog()));
  rcl->addWidget(openCamera);

  isovalue_properties = new QFrame;
  isovalue_properties->setVisible(false);
  QHBoxLayout *rl = new QHBoxLayout;
  isovalue_properties->setLayout(rl);
  rl->addWidget(new QLabel("isovalue"));
  isovalue = new QLineEdit;
  isovalue->setValidator(new QDoubleValidator());
  rl->addWidget(isovalue);

  rcl->addWidget(isovalue_properties);

  gradient_properties = new QFrame;
  gradient_properties->setVisible(false);
  rl = new QHBoxLayout;
  gradient_properties->setLayout(rl);
  rl->addWidget(new QLabel("gradient"));
  gradient = new QLineEdit;
  gradient->setValidator(new QDoubleValidator());
  rl->addWidget(gradient);

  rcl->addWidget(gradient_properties);

  layout->addWidget(raycast_properties);

  mh_properties = new QFrame;
  mh_properties->setVisible(true);
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
SamplerModel::nPorts(QtNodes::PortType portType) const
{
  if (portType == QtNodes::PortType::In)
    return 1;
  else
    return 1;
}

QtNodes::NodeDataType
SamplerModel::dataType(QtNodes::PortType pt, QtNodes::PortIndex pi) const
{
  return GxyData().type();
}

void
SamplerModel::onApply()
{
  QJsonObject sampleJson;
  sampleJson["cmd"] = "gui::sample";
  sampleJson["Camera"] = camera.save();
}

std::shared_ptr<QtNodes::NodeData>
SamplerModel::outData(QtNodes::PortIndex)
{
  std::shared_ptr<GxyData> result;
  return std::static_pointer_cast<QtNodes::NodeData>(result);
}

QtNodes::NodeValidationState
SamplerModel::validationState() const
{
  return QtNodes::NodeValidationState::Valid;
}


QString
SamplerModel::validationMessage() const
{
  return QString("copacetic");
}

void
SamplerModel::setInData(std::shared_ptr<QtNodes::NodeData> data, QtNodes::PortIndex portIndex)
{
  input = std::dynamic_pointer_cast<GxyData>(data);
  
  if (input)
    loadInputDrivenWidgets(std::dynamic_pointer_cast<GxyPacket>(input));

  enableIfValid();
}

void
SamplerModel::loadInputDrivenWidgets(std::shared_ptr<GxyPacket> o) const
{
  std::shared_ptr<GxyData> input = std::dynamic_pointer_cast<GxyData>(o);
  mh_linear_tf_min->setText(QString::number(input->dataInfo.data_min));
  mh_linear_tf_max->setText(QString::number(input->dataInfo.data_max));
}

void
SamplerModel::loadParameterWidgets() const
{
}

bool
SamplerModel::isValid()
{
  return input && input->isValid();
}



