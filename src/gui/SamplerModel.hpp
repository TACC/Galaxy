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


#include <QtCore/QObject>
#include <QtCore/QVector>

#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>

#include "GxyModel.hpp"

#include "GxyData.hpp"
#include "Camera.hpp"
#include "CameraDialog.hpp"

#include "Lights.hpp"
#include "LightsDialog.hpp"

class SamplerModel : public GxyModel
{
  Q_OBJECT

public:
  SamplerModel();

  virtual
  ~SamplerModel() {}

  unsigned int nPorts(QtNodes::PortType portType) const override;

  QtNodes::NodeDataType dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override;

  std::shared_ptr<QtNodes::NodeData> outData(QtNodes::PortIndex port) override;

  void setInData(std::shared_ptr<QtNodes::NodeData> data, QtNodes::PortIndex portIndex) override;

  QtNodes::NodeValidationState validationState() const override;

  QString validationMessage() const override;

  QString caption() const override { return QStringLiteral("Sampler"); }

  QString name() const override { return QStringLiteral("Sampler"); }

public Q_SLOTS: 

  virtual void onApply() override;

private Q_SLOTS:

  void algorithmChanged(int t)
  {
    std::cerr << "alg " << t << "\n";
    openCamera->setEnabled(t != 0);
    mh_properties->setVisible(t == 0);
    raycast_properties->setVisible(t != 0);
    gradient_properties->setVisible(t == 1);
    isovalue_properties->setVisible(t == 2);
  }

  void mh_tfunc_Changed(int t)
  {
    std::cerr << t << "\n";
    linear->setVisible(t == 1);
    gaussian->setVisible(t == 2);
  }

  void openCameraDialog() 
  {
    CameraDialog *cameraDialog = new CameraDialog(camera);
    cameraDialog->exec();
    cameraDialog->get_camera(camera);
    delete cameraDialog;
  }

private:

  Camera camera;

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

  QFrame *raycast_properties;

  QFrame *isovalue_properties;
  QLineEdit *isovalue;

  QFrame *gradient_properties;
  QLineEdit *gradient;

  QComboBox *type;
  QPushButton *openCamera;
  QLineEdit *parameter;
  std::shared_ptr<GxyData> output;

  

};
