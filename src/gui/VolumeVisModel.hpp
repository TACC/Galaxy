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
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QFileDialog>

#include "GxyModel.hpp"

using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeDataModel;
using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeValidationState;


using QtNodes::NodeDataModel;
using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeValidationState;

#include "GxyData.hpp"
#include "PlanesDialog.hpp"
#include "ScalarsDialog.hpp"

#include "VolumeVis.hpp"

class VolumeVisModel : public GxyModel
{
  Q_OBJECT

public:
  VolumeVisModel();

  virtual

  ~VolumeVisModel() {}

  unsigned int nPorts(PortType portType) const override;

  NodeDataType dataType(PortType portType, PortIndex portIndex) const override;

  std::shared_ptr<NodeData> outData(PortIndex port) override;

  void setInData(std::shared_ptr<NodeData> data, PortIndex portIndex) override;

  NodeValidationState validationState() const override;

  QString validationMessage() const override;

  QString caption() const override { return QStringLiteral("VolumeVis"); }

  QString name() const override { return QStringLiteral("VolumeVis"); }


private Q_SLOTS:

  void onApply();

  void openPlanesDialog() 
  {
    PlanesDialog *planesDialog = new PlanesDialog(slices);
    planesDialog->exec();
    slices = planesDialog->get_planes();
    delete planesDialog;
  }

  void openIsovaluesDialog() 
  {
    ScalarsDialog *scalarsDialog = new ScalarsDialog(isovalues);
    scalarsDialog->exec();
    isovalues = scalarsDialog->get_scalars();
    delete scalarsDialog;
  }

  void openFileSelectorDialog()
  {
    QFileDialog *fileDialog = new QFileDialog();
    fileDialog->exec();
    tf_widget->clear();
    tf_widget->insert(fileDialog->selectedFiles().at(0));
    std::cerr << tf_widget->text().toStdString() << "\n";
    delete fileDialog;
  }

public:

  std::vector<gxy::vec4f>  slices;
  std::vector<float>       isovalues;
  bool                     volume_rendering_flag;
  std::string              transfer_function;

private:
  std::shared_ptr<VolumeVis> output;
  std::shared_ptr<GxyData> input;


  QLineEdit                *tf_widget;
};
