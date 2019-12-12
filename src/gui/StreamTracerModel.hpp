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

#include "GxyModel.hpp"

class StreamTracerModel : public GxyModel
{
  Q_OBJECT

public:
  StreamTracerModel();

  virtual
  ~StreamTracerModel() {}

  unsigned int nPorts(QtNodes::PortType portType) const override;

  QtNodes::NodeDataType dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override;

  std::shared_ptr<QtNodes::NodeData> outData(QtNodes::PortIndex port) override;

  void setInData(std::shared_ptr<QtNodes::NodeData> data, QtNodes::PortIndex portIndex) override;

  QtNodes::NodeValidationState validationState() const override;

  QString validationMessage() const override;

  QString caption() const override { return QStringLiteral("StreamTracer"); }

  QString name() const override { return QStringLiteral("StreamTracer"); }

  bool isValid() override;

  QJsonObject save() const override;
  void restore(QJsonObject const &p) override;

public Q_SLOTS:

  virtual void onApply() override;

protected:

  std::shared_ptr<GxyData>  vectorField;
  std::shared_ptr<GxyData>  seeds;

private:

  QLineEdit               *trimtime;
  QLineEdit               *trimdeltatime;
  QLineEdit               *maxsteps;
  QLineEdit               *stepsize;
  QLineEdit               *minvelocity;
  QLineEdit               *maxtime;
  std::shared_ptr<GxyData> output;

  bool retrace = true;
  bool retrim  = true;
};