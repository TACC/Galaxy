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

#include <iostream>

#include "ImporterModel.hpp"
#include "GxyData.hpp"

ImporterModel::ImporterModel() 
{
  QFrame *frame = new QFrame();

  QVBoxLayout *outer_layout = new QVBoxLayout();
  outer_layout->setSpacing(0);
  outer_layout->setContentsMargins(2, 0, 2, 0);

  QWidget *fsBox = new QWidget();
  QHBoxLayout *fsLayout = new QHBoxLayout();
  fsBox->setLayout(fsLayout);

  fileName = new QLineEdit();
  fsLayout->addWidget(fileName);
  
  QPushButton *openFileSelector = new QPushButton("Select File");
  connect(openFileSelector, SIGNAL(released()), this, SLOT(openFileSelectorDialog()));
  fsLayout->addWidget(openFileSelector);

  outer_layout->addWidget(fsBox);

  QFrame *typeFrame = new QFrame();
  QVBoxLayout *typeLayout = new QVBoxLayout();
  typeFrame->setLayout(typeLayout);

  QButtonGroup *typeButtonGroup = new QButtonGroup();
  connect(typeButtonGroup, QOverload<int>::of(&QButtonGroup::buttonClicked), this, &ImporterModel::setDataType);

  QRadioButton *button = new QRadioButton("Volume");
  typeLayout->addWidget(button);
  typeButtonGroup->addButton(button, 0);
  
  button = new QRadioButton("Surface");
  typeLayout->addWidget(button);
  typeButtonGroup->addButton(button, 1);
  
  button = new QRadioButton("Points");
  typeLayout->addWidget(button);
  typeButtonGroup->addButton(button, 2);

  outer_layout->addWidget(typeFrame);

  QFrame *dataNameFrame = new QFrame();
  QHBoxLayout *dataName_layout = new QHBoxLayout();
  dataNameFrame->setLayout(dataName_layout);

  dataName = new QLineEdit(QUuid::createUuid().toString());
  dataName->hide();

  QCheckBox *transientClickBox = new QCheckBox("Transient");
  transientClickBox->setChecked(true);
  connect(transientClickBox, SIGNAL(stateChanged(int)), this, SLOT(transientChanged(int)));

  dataName_layout->addWidget(transientClickBox);
  dataName_layout->addWidget(dataName);

  outer_layout->addWidget(dataNameFrame);
  frame->setLayout(outer_layout);
  
  _container->setCentralWidget(frame);
}

unsigned int
ImporterModel::nPorts(PortType portType) const
{
  return (portType == PortType::In) ? 0 : 1;
}

NodeDataType
ImporterModel::dataType(PortType, PortIndex) const
{
  return GxyData().type();
}

void
ImporterModel::apply() { std::cerr << "Apply\n"; }

std::shared_ptr<NodeData>
ImporterModel::outData(PortIndex)
{
  std::shared_ptr<GxyData> result;
  return std::static_pointer_cast<NodeData>(result);
}

void
ImporterModel::
setInData(std::shared_ptr<NodeData> data, PortIndex portIndex)
{}

NodeValidationState
ImporterModel::validationState() const
{
  return NodeValidationState::Valid;
}

QString
ImporterModel::validationMessage() const
{
  return QString("copacetic");
}

