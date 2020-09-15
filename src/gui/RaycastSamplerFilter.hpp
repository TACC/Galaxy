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

class RaycastSamplerFilter : public GxyFilter
{
  Q_OBJECT

public:
  RaycastSamplerFilter();

  virtual
  ~RaycastSamplerFilter() {}

  unsigned int nPorts(QtNodes::PortType portType) const override;

  QtNodes::NodeDataType dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override;

  void setInData(std::shared_ptr<QtNodes::NodeData> data, QtNodes::PortIndex portIndex) override;

  QtNodes::NodeValidationState validationState() const override;

  QString validationMessage() const override;

  QString caption() const override { return QStringLiteral("RaycastSampler"); }

  QString name() const override { return QStringLiteral("RaycastSampler"); }

  virtual void loadInputDrivenWidgets(std::shared_ptr<GxyPacket> o) override;
  bool isValid() override;

  QJsonObject save() const override;
  void restore(QJsonObject const &p) override;

public Q_SLOTS: 

  virtual void onApply() override;

private Q_SLOTS:

  void algorithmChanged(int t)
  {
    gradient_properties->setVisible(t == 1);
    isovalue_properties->setVisible(t == 0);
  }

  void openCameraDialog() 
  {
    CameraDialog *cameraDialog = new CameraDialog(camera);
    cameraDialog->exec();
    cameraDialog->get_camera(camera);
    delete cameraDialog;
  }

private:

  std::shared_ptr<GxyData> input;
  // std::shared_ptr<GxyData> output;

  Camera camera;

  QFrame *raycast_properties;

  QFrame *isovalue_properties;
  QLineEdit *isovalue;

  QFrame *gradient_properties;
  QLineEdit *tolerance;

  QComboBox *type;
  QPushButton *openCamera;
};
