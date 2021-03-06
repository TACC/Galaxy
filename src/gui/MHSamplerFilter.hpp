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

#pragma once


#include <QtCore/QObject>
#include <QtCore/QVector>

#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>

#include "GxyFilter.hpp"

#include "GxyData.hpp"
#include "Camera.hpp"
#include "CameraDialog.hpp"

#include "Lights.hpp"
#include "LightsDialog.hpp"

#include <nodes/NodeData>

class MHSamplerFilter : public GxyFilter
{
  Q_OBJECT

public:
  MHSamplerFilter();

  virtual
  ~MHSamplerFilter() {}

  unsigned int nPorts(QtNodes::PortType portType) const override;

  QtNodes::NodeDataType dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override;

  std::shared_ptr<QtNodes::NodeData> outData(QtNodes::PortIndex port) override;

  void setInData(std::shared_ptr<QtNodes::NodeData> data, QtNodes::PortIndex portIndex) override;

  QtNodes::NodeValidationState validationState() const override;

  QString validationMessage() const override;

  QString caption() const override { return QStringLiteral("MHSampler"); }

  QString name() const override { return QStringLiteral("MHSampler"); }

  virtual void loadInputDrivenWidgets(std::shared_ptr<GxyPacket> o) override;
  bool isValid() override;

  QJsonObject save() const override;
  void restore(QJsonObject const &p) override;

public Q_SLOTS: 

  virtual void onApply() override;

private Q_SLOTS:

  void mh_tfunc_Changed(int t)
  {
    linear->setVisible(t == 1);
    gaussian->setVisible(t == 2);
  }

private:

  std::shared_ptr<GxyData> input;
  std::shared_ptr<GxyData> output;

  QFrame *mh_properties;
  QComboBox *mh_tfunc;
  QLineEdit *mh_radius;
  QLineEdit *mh_sigma;
  QLineEdit *mh_iterations;
  QLineEdit *mh_startup;
  QLineEdit *mh_skip;
  QLineEdit *mh_miss;

  QFrame *linear;
  QLineEdit *mh_linear_tf_min;
  QLineEdit *mh_linear_tf_max;

  QFrame *gaussian;
  QLineEdit *gaussian_mean;
  QLineEdit *gaussian_std;
};
