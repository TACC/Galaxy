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

#include "ParticlesVisModel.hpp"

ParticlesVisModel::ParticlesVisModel() 
{
  QFrame *frame  = new QFrame();
  QVBoxLayout *layout = new QVBoxLayout();
  layout->setSpacing(0);
  layout->setContentsMargins(2, 0, 2, 0);
  frame->setLayout(layout);

  QWidget     *radius_box = new QWidget();
  QGridLayout *radius_box_layout = new QGridLayout();
  radius_box->setLayout(radius_box_layout);

  radius_box_layout->addWidget(new QLabel("data range"), 0, 0);

  useFullRange = new QCheckBox("full");
  useFullRange->setChecked(true);
  connect(useFullRange, SIGNAL(stateChanged(int)), this, SLOT(useFullRangeChanged(int)));
  radius_box_layout->addWidget(useFullRange, 0, 1);

  minrange = new QLineEdit();
  minrange->setValidator(new QDoubleValidator());
  minrange->setText(QString::number(0.0));
  minrange->setEnabled(false);
  radius_box_layout->addWidget(minrange, 0, 2);

  maxrange = new QLineEdit();
  maxrange->setValidator(new QDoubleValidator());
  maxrange->setText(QString::number(1.0));
  maxrange->setEnabled(false);
  radius_box_layout->addWidget(maxrange, 0, 3);

  radius_box_layout->addWidget(new QLabel("radius range"), 1, 0);

  minradius = new QLineEdit();
  minradius->setValidator(new QDoubleValidator());
  minradius->setText(QString::number(0.0));
  minradius->setEnabled(true);
  radius_box_layout->addWidget(minradius, 1, 2);

  maxradius = new QLineEdit();
  maxradius->setValidator(new QDoubleValidator());
  maxradius->setText(QString::number(1.0));
  maxradius->setEnabled(true);
  radius_box_layout->addWidget(maxradius, 1, 3);

  layout->addWidget(radius_box);

  QFrame *cmap_box = new QFrame();
  QHBoxLayout *cmap_box_layout = new QHBoxLayout();
  cmap_box->setLayout(cmap_box_layout);

  cmap_box_layout->addWidget(new QLabel("tfunc"));

  tf_widget = new QLineEdit();
  cmap_box_layout->addWidget(tf_widget);
  
  QPushButton *tfunc_browse_button = new QPushButton("...");
  cmap_box_layout->addWidget(tfunc_browse_button);
  connect(tfunc_browse_button, SIGNAL(released()), this, SLOT(openFileSelectorDialog()));

  layout->addWidget(cmap_box);
  
  _container->setCentralWidget(frame);
}

unsigned int
ParticlesVisModel::nPorts(PortType portType) const
{
  return 1; // PortType::In or ::Out
}

NodeDataType
ParticlesVisModel::dataType(PortType pt, PortIndex) const
{
  if (pt == PortType::In)
    return GxyData().type();
  else
    return JsonVis().type();
}

void
ParticlesVisModel::apply() { std::cerr << "Apply\n"; }

std::shared_ptr<NodeData>
ParticlesVisModel::outData(PortIndex)
{
  std::shared_ptr<Json> result;
  return std::static_pointer_cast<NodeData>(result);
}

void
ParticlesVisModel::
setInData(std::shared_ptr<NodeData> data, PortIndex portIndex)
{
  // volumeData = std::dynamic_pointer_cast<GxyData>(data);
}


NodeValidationState
ParticlesVisModel::validationState() const
{
  return NodeValidationState::Valid;
}


QString
ParticlesVisModel::validationMessage() const
{
  return QString("copacetic");
}

