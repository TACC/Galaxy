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

#include <QtGui/QDoubleValidator>

#include "GxyModel.hpp"

using QtNodes::NodeDataModel;
using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeValidationState;


using QtNodes::NodeDataModel;
using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeValidationState;

#include "Json.hpp"
#include "GxyData.hpp"

class PathlinesVisModel : public GxyModel
{
  Q_OBJECT

public:
  PathlinesVisModel();

  virtual
  ~PathlinesVisModel() {}

  unsigned int nPorts(PortType portType) const override;

  NodeDataType dataType(PortType portType, PortIndex portIndex) const override;

  std::shared_ptr<NodeData> outData(PortIndex port) override;

  void setInData(std::shared_ptr<NodeData> data, PortIndex portIndex) override;

  NodeValidationState validationState() const override;

  QString validationMessage() const override;

  QString caption() const override { return QStringLiteral("PathlinesVis"); }

  QString name() const override { return QStringLiteral("PathlinesVis"); }

  QWidget *embeddedWidget() override { return _container; }

protected:

  virtual void apply();

private Q_SLOTS:

  void useFullRangeChanged(int s)
  {
    minrange->setEnabled(s == 0);
    maxrange->setEnabled(s == 0);
  }

  void openFileSelectorDialog()
  {
    QFileDialog *fileDialog = new QFileDialog();
    fileDialog->exec();
    tf_widget->clear();
    tf_widget->insert(fileDialog->selectedFiles().at(0));
    delete fileDialog;
  }

private:

  QLineEdit               *tf_widget;

  QCheckBox               *useFullRange;
  QLineEdit               *minrange;
  QLineEdit               *maxrange;
  QLineEdit               *minradius;
  QLineEdit               *maxradius;
};