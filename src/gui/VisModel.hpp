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
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QFileDialog>

#include <QtGui/QDoubleValidator>

#include "GxyModel.hpp"

using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeDataModel;
using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeValidationState;

#include "GxyData.hpp"
#include "PlanesDialog.hpp"
#include "ScalarsDialog.hpp"

#include <QJsonArray>

#include "Vis.hpp"

class VisModel : public GxyModel
{
  Q_OBJECT

public:
  VisModel();
  ~VisModel() {}

  unsigned int nPorts(PortType portType) const override;

  NodeValidationState validationState() const override;

  QString validationMessage() const override;

  QString caption() const override { return QStringLiteral("Vis"); }

  QString name() const override { return QStringLiteral("Vis"); }

  QJsonObject save() const override;

  void setInData(std::shared_ptr<NodeData> data, PortIndex portIndex) override
  {
    std::cerr << "setInData on virtual VisModel\n";
    exit(1);
  }

  virtual void loadInputDrivenWidgets(std::shared_ptr<GxyPacket> o) const override;
  virtual void loadParameterWidgets(std::shared_ptr<GxyPacket> o) const override;

  virtual void loadOutput(std::shared_ptr<GxyPacket> o) const override;

  bool isValid() override;

private Q_SLOTS:

  void onApply() override;

  void onDataRangeReset();

  void openCmapSelectorDialog()
  {
    QFileDialog *fileDialog = new QFileDialog();
    fileDialog->setNameFilter(tr("Colormaps (*.cmap *.json)"));
    fileDialog->exec();
    cmap_widget->clear();
    cmap_widget->insert(fileDialog->selectedFiles().at(0));
    delete fileDialog;
    enableIfValid();
  }

protected:
  std::shared_ptr<GxyData> input;
  std::shared_ptr<GxyPacket> output;

private:
  QLineEdit *cmap_widget = NULL;
  QLineEdit *cmap_range_min = NULL;
  QLineEdit *cmap_range_max = NULL;
};