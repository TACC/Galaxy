// ========================================================================== /
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

#include <iostream>

#include "DataSourceModel.hpp"
#include "GxyData.hpp"

DataSourceModel::DataSourceModel() 
{
  DataSourceModel::init();

  output = std::make_shared<GxyData>(getModelIdentifier());

  QFrame *frame = new QFrame();

  QVBoxLayout *outer_layout = new QVBoxLayout();
  outer_layout->setSpacing(0);
  outer_layout->setContentsMargins(2, 0, 2, 0);

  objectList = new MyQListWidget;
  connect(objectList, SIGNAL(itemClicked(QListWidgetItem *)), this, SLOT(selection(QListWidgetItem *)));
  outer_layout->addWidget(objectList);

  frame->setLayout(outer_layout);
  
  _properties->addProperties(frame);

  QPushButton *add = new QPushButton("Add");
  connect(add, SIGNAL(released()), this, SLOT(onAdd()));
  _properties->addButton(add);

  info = new QPushButton("Info");
  info->setEnabled(false);
  connect(info, SIGNAL(released()), objectList, SLOT(showDialog()));
  _properties->addButton(info);

  QPushButton *refresh = new QPushButton("Refresh");
  connect(refresh, SIGNAL(released()), this, SLOT(onRefresh()));
  _properties->addButton(refresh);

  connect(_properties->getApplyButton(), SIGNAL(released()), this, SLOT(onApply()));

  GxyConnectionMgr *gxyMgr = getTheGxyConnectionMgr();
  if (gxyMgr->IsConnected())
    onRefresh();

  connect(_properties->getApplyButton(), SIGNAL(released()), this, SLOT(onApply()));
  connect(getTheGxyConnectionMgr(), SIGNAL(connectionStateChanged(bool)), this, SLOT(onRefresh()));

  Observe(gxyMgr);
}

void 
DataSourceModel::Notify(Observer *sender, Observer::ObserverEvent event, void* cargo)
{
  rapidjson::Document msg;
  if (cargo)
  {
    msg.Parse((const char *)cargo);
    std::string action = msg["action"].GetString();
    std::string argument = msg["argument"].GetString();

    if (current_selection != NULL && current_selection->getDataInfo().name == argument)
    {
      if (isValid())
        QMetaObject::invokeMethod(this, "onApply");
      else
        std::cerr << "didn't apply - not valid\n";
    }
    else
    {
      bool has = false;
      for (int i = 0; !has && i < objectList->count(); i++)
        has = (((MyQListWidgetItem *)objectList->item(i))->getDataInfo().name == argument);

      if (!has)
      {
        std::cerr << "REFRESH\n";
        onRefresh();
      }
      else
        std::cerr << "known, but not mine\n";
    }
  }
  // else
    // std::cerr << "DataSourceModel::Notify without cargo\n";
}


unsigned int
DataSourceModel::nPorts(PortType portType) const
{
  return (portType == PortType::In) ? 0 : 1;
}

NodeDataType
DataSourceModel::dataType(PortType, PortIndex) const
{
  return GxyData().type();
}

NodeValidationState
DataSourceModel::validationState() const
{
  return NodeValidationState::Valid;
}

QString
DataSourceModel::validationMessage() const
{
  return QString("copacetic");
}

