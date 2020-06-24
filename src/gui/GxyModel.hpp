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
#include <QtWidgets/QFrame>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>

#include <nodes/NodeDataModel>

using QtNodes::NodeDataModel;

#include "GxyData.hpp"
#include "Properties.hpp"
#include "Observer.h"

class GxyModel : public NodeDataModel, public Observer
{
  Q_OBJECT

public:
  GxyModel()
  {
    model_identifier = QUuid::createUuid().toString().toStdString();

    frame = new QFrame();
    layout = new QVBoxLayout;
    layout->setSpacing(0);
    layout->setContentsMargins(2, 0, 2, 0);
    frame->setLayout(layout);

    _properties = new Properties(this, frame);

    QPushButton *openPropertiesButton = new QPushButton("Properties") ;
    connect(openPropertiesButton, SIGNAL(released()), this, SLOT(openProperties()));
    layout->addWidget(openPropertiesButton);

    applyButton = new QPushButton("Apply");
    connect(applyButton, SIGNAL(released()), this, SLOT(onApply()));
    connect(_properties->getApplyButton(), SIGNAL(released()), this, SLOT(onApply()));
    layout->addWidget(applyButton);

    enable(false);
  }

  virtual
  ~GxyModel() {}

  QWidget *embeddedWidget() override { return frame; }
  std::string getModelIdentifier() { return model_identifier; }

  QJsonObject
  save() const override
  {
    QJsonObject modelJson = NodeDataModel::save();

    if (label)
      modelJson["label"] = label->text();

    modelJson["id"] = model_identifier.c_str();

    return modelJson;
  }

  void
  restore(QJsonObject const &p) override
  {
    NodeDataModel::restore(p);

    QJsonValue l = p["label"];
    if (!l.isUndefined())
    {
      addLabel();
      label->setText(l.toString());
    }
  }

  void 
  addLabel()
  {
    if (! label)
    {
      label = new QLineEdit;
      layout->insertWidget(0, label);
    }
  }

  
  virtual void loadInputDrivenWidgets(std::shared_ptr<GxyPacket> p) 
  {
  }

  // This is used to set widgets from internal state.   Only necessary if the
  // internal state is not held in the widgets in the first place.

  virtual void loadParameterWidgets() const
  {
  }

  virtual void loadOutput(std::shared_ptr<GxyData> p) const 
  {
    p->dataInfo = input->dataInfo;
  }

  void setInData(std::shared_ptr<QtNodes::NodeData> data, QtNodes::PortIndex portIndex) override
  {
    if (isValid())
    {
      enable(true);
      Q_EMIT dataUpdated(0);
    }
    else
    {
      enable(false);
    }
  }

  virtual bool isValid() { return true; }

public Q_SLOTS:

  void openProperties() { _properties->show(); _properties->raise(); }

  void enable(bool b)
  {
    applyButton->setEnabled(b);
    _properties->getApplyButton()->setEnabled(b);
  }

  void enableIfValid()
  {
    enable(isValid());
  }


  virtual void onApply() 
  {
    Q_EMIT dataUpdated(0);
  }

protected:
  QVBoxLayout *layout;
  QLineEdit *label = NULL;
  QPushButton *applyButton;
  Properties *_properties;
  QFrame *frame;
  std::string model_identifier;
  std::shared_ptr<GxyData> input;
};
