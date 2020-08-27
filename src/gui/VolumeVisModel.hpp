// ========================================================================== //
//                                                                            //
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
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QFileDialog>
#include <QtGui/QDoubleValidator>
#include <QJsonArray>

#include "PlanesDialog.hpp"
#include "ScalarsDialog.hpp"
#include "VisModel.hpp"
#include "VolumeVis.hpp"

using QtNodes::Node;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeDataModel;
using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeValidationState;

class VolumeVisModel : public VisModel
{
  Q_OBJECT

public:
  VolumeVisModel();
  ~VolumeVisModel();

  unsigned int nPorts(PortType portType) const override;

  NodeDataType dataType(PortType portType, PortIndex portIndex) const override;

  std::shared_ptr<NodeData> outData(PortIndex port) override;

  void setInData(std::shared_ptr<NodeData> data, PortIndex portIndex) override;

  NodeValidationState validationState() const override;

  QString validationMessage() const override;

  QString caption() const override { return QStringLiteral("VolumeVis"); }

  QString name() const override { return QStringLiteral("VolumeVis"); }

  QJsonObject save() const override;
  void restore(QJsonObject const &p) override;

  bool isValid() override;

  virtual void loadInputDrivenWidgets(std::shared_ptr<GxyPacket> o) override;
  virtual void loadParameterWidgets() override;

  virtual void loadOutput(std::shared_ptr<GxyData> o) const override;

private Q_SLOTS:

  void volume_rendering_flag_state_changed(int b) 
  {
    geomWidgets->setEnabled(b == 0);
    enableIfValid();
  }

  void onApply() override;

  void disableApply()
  {
     _properties->getApplyButton()->setEnabled(false);
  }
    
  void enableApply()
  {
     _properties->getApplyButton()->setEnabled(true);
  }
    
  void onDataRangeReset();

  void openPlanesDialog() 
  {
    slicesDialog->exec();
  }

  void openIsovaluesDialog() 
  {
    isovaluesDialog->exec();
  }

private:
  ScalarsDialog *isovaluesDialog;
  PlanesDialog  *slicesDialog;

  QCheckBox *volumeRender = NULL;
  QFrame *geomWidgets = NULL;
};
