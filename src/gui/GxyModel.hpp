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
#include <QtCore/QUuid>

#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QCheckBox>

#include <nodes/NodeDataModel>

using QtNodes::NodeDataModel;

#include "ParameterFrame.hpp"


class GxyModel : public NodeDataModel
{
  Q_OBJECT

public:
  GxyModel()
  {
    model_identifier = QUuid::createUuid().toString().toStdString();
    _container = new ParameterFrame(this);
  }

  virtual
  ~GxyModel() {}

  QWidget *embeddedWidget() override { return _container; }
  std::string getModelIdentifier() { return model_identifier; }

private Q_SLOTS:

protected:
  ParameterFrame *_container;
  std::string model_identifier;
};
