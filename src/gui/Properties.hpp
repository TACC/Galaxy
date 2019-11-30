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
#include <iostream>


#include <nodes/NodeDataModel>
using QtNodes::NodeDataModel;

#include <QtCore/QVector>
#include <QFont>
#include <QSizePolicy>

#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QFrame>
#include <QtWidgets/QPushButton>

#include <QtGui/QDoubleValidator>

class Properties : public QWidget
{
  Q_OBJECT;

public:
  Properties(NodeDataModel *p, QWidget *parent) 
  {
    model = p;

    outer_layout = new QVBoxLayout();
    outer_layout->setSpacing(0);
    outer_layout->setContentsMargins(0, 0, 0, 0);

    panel = new QFrame(parent);

    panel->setFrameStyle(QFrame::Panel | QFrame::Raised);
    panel->setLineWidth(2);
    panel_layout = new QVBoxLayout();
    panel_layout->setSpacing(0);
    panel_layout->setContentsMargins(0, 0, 0, 0);
    panel->setLayout(panel_layout);

    outer_layout->addWidget(panel);

    QWidget *bottom_right = new QWidget();
    QHBoxLayout *bottom_right_layout = new QHBoxLayout();
    bottom_right_layout->setContentsMargins(0, 0, 0, 5);
    bottom_right_layout->setSpacing(0);
    bottom_right->setLayout(bottom_right_layout);

    applyButton = new QPushButton("Apply");
    applyButton->setEnabled(false);
    bottom_right_layout->addWidget(applyButton, 0, Qt::AlignRight);

    doneButton = new QPushButton("Done");
    connect(doneButton, SIGNAL(released()), this, SLOT(close()));
    bottom_right_layout->addWidget(doneButton, 0, Qt::AlignRight);

    QWidget *bottom_left = new QWidget();
    bottom_left_layout = new QHBoxLayout();
    bottom_left_layout->setContentsMargins(0, 0, 0, 5);
    bottom_left_layout->setSpacing(0);
    bottom_left->setLayout(bottom_left_layout);

    QWidget *bottom = new QWidget();
    QHBoxLayout *bottom_layout = new QHBoxLayout();
    bottom->setLayout(bottom_layout);

    bottom_layout->addWidget(bottom_left);
    bottom_layout->addWidget(bottom_right);

    outer_layout->addWidget(bottom);
    setLayout(outer_layout);
  }

  void addProperties(QFrame *w)
  {
    w->setFrameStyle(QFrame::Panel | QFrame::Raised);
    panel_layout->addWidget(w);
  }

  void addButton(QPushButton *w)
  {
    w->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    bottom_left_layout->addWidget(w);
  }

  QPushButton *getApplyButton() { return applyButton; }

signals:

  void apply();

private:
  NodeDataModel *model;
  QPushButton *doneButton;
  QPushButton *applyButton;
  int toggle = 0;
  QHBoxLayout *bottom_left_layout;
  QFrame *panel;
  QVBoxLayout *panel_layout;
  QVBoxLayout *outer_layout;
};
