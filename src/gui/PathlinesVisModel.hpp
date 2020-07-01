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

#include "dtypes.h"

#include <vector>
#include <string>

#include <QtCore/QObject>

#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QFileDialog>

#include <QtGui/QDoubleValidator>

#include "VisModel.hpp"
#include "GxyData.hpp"

class PathlinesVisModel : public VisModel
{
  Q_OBJECT

public:
  PathlinesVisModel();

  virtual
  ~PathlinesVisModel() {}

  unsigned int nPorts(QtNodes::PortType portType) const override;

  QtNodes::NodeDataType dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override;

  std::shared_ptr<QtNodes::NodeData> outData(QtNodes::PortIndex port) override;

  void setInData(std::shared_ptr<QtNodes::NodeData> data, QtNodes::PortIndex portIndex) override;

  QtNodes::NodeValidationState validationState() const override;

  QString validationMessage() const override;

  QString caption() const override { return QStringLiteral("PathlinesVis"); }

  QString name() const override { return QStringLiteral("PathlinesVis"); }

  QJsonObject save() const override;
  void restore(QJsonObject const &p) override;

  bool isValid() override;

  virtual void loadInputDrivenWidgets(std::shared_ptr<GxyPacket> o) override;
  virtual void loadParameterWidgets() const override;

  virtual void loadOutput(std::shared_ptr<GxyData> o) const override;

  void onApply() override;

private:

  QLineEdit               *minrange;
  QLineEdit               *maxrange;
  QLineEdit               *minradius;
  QLineEdit               *maxradius;
};
