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

#include <iostream>


#include <QtCore/QObject>
#include <QtCore/QVector>
#include <QtCore/QUuid>

#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QVBoxLayout>

#include <nodes/NodeDataModel>

using QtNodes::NodeDataModel;
using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeValidationState;

#include "GxyModel.hpp"
#include "GxyData.hpp"

class ImporterModel : public GxyModel
{
  Q_OBJECT

public:
  ImporterModel();

  virtual
  ~ImporterModel() {}

  unsigned int nPorts(PortType portType) const override;

  NodeDataType dataType(PortType portType, PortIndex portIndex) const override;

  std::shared_ptr<NodeData> outData(PortIndex port) override;

  void setInData(std::shared_ptr<NodeData> data, PortIndex portIndex) override;

  NodeValidationState validationState() const override;

  QString validationMessage() const override;

  QString caption() const override { return QStringLiteral("Importer"); }

  QString name() const override { return QStringLiteral("Importer"); }

  QWidget *embeddedWidget() override { return _container; }

protected:

  virtual void apply();

private Q_SLOTS:

  void openFileSelectorDialog()
  {
    QFileDialog *fileDialog = new QFileDialog();
    fileDialog->exec();
    fileName->clear();
    fileName->insert(fileDialog->selectedFiles().at(0));
    std::cerr << fileName->text().toStdString() << "\n";
    delete fileDialog;
  }

  void setDataType(int t)
  {
    type = t;
  }

  void transientChanged(int t)
  {
    if (t) dataName->hide();
    else dataName->show();
  }

private:

  int type = -1;
  QLineEdit *fileName;
  QLineEdit *dataName;
  std::weak_ptr<GxyData> data;
};
