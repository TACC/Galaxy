
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

  objectList = new QListWidget();
  connect(objectList, SIGNAL(itemPressed(QListWidgetItem *)), this, SLOT(selection(QListWidgetItem *)));
  outer_layout->addWidget(objectList);

  frame->setLayout(outer_layout);
  
  _container->setCentralWidget(frame);

  QPushButton *add = new QPushButton("Add");
  connect(add, SIGNAL(released()), this, SLOT(onAdd()));
  _container->addButton(add);

  QPushButton *refresh = new QPushButton("Refresh");
  connect(refresh, SIGNAL(released()), this, SLOT(onRefresh()));
  _container->addButton(refresh);

  connect(_container->getApplyButton(), SIGNAL(released()), this, SLOT(onApply()));

  GxyConnectionMgr *gxyMgr = getTheGxyConnectionMgr();
  if (gxyMgr->IsConnected())
    onRefresh();

  connect(_container->getApplyButton(), SIGNAL(released()), this, SLOT(onApply()));
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

std::shared_ptr<NodeData>
DataSourceModel::outData(PortIndex)
{
  return std::static_pointer_cast<NodeData>(output);
}

void
DataSourceModel::
setInData(std::shared_ptr<NodeData> data, PortIndex portIndex)
{}

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

