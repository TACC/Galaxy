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
#include <QtWidgets/QCheckBox>

#include <QtGui/QDoubleValidator>

class ParameterFrame : public QWidget
{
  Q_OBJECT;

public:
  ParameterFrame(NodeDataModel *p) 
  {
    parent = p;
  }

  void setCentralWidget(QFrame *w)
  {
    panel = w;

    QVBoxLayout *outer_layout = new QVBoxLayout();
    outer_layout->setSpacing(0);
    outer_layout->setContentsMargins(0, 0, 0, 0);

    openClosed = new QPushButton("v");
    QFont font("Times", 10, QFont::Bold);
    openClosed->setFont(font);
    openClosed->setChecked(true);
    connect(openClosed, SIGNAL(released()), this, SLOT(toggleState()));
    outer_layout->addWidget(openClosed);
    outer_layout->setAlignment(openClosed, Qt::AlignLeft);

    outer_layout->addWidget(panel);
    w->setFrameStyle(QFrame::Panel | QFrame::Raised);
    w->setLineWidth(2);

    QWidget *bottom_right = new QWidget();
    QHBoxLayout *bottom_right_layout = new QHBoxLayout();
    bottom_right_layout->setContentsMargins(0, 0, 0, 5);
    bottom_right_layout->setSpacing(0);
    bottom_right->setLayout(bottom_right_layout);

    applyButton = new QPushButton("Apply");
    bottom_right_layout->addWidget(applyButton, 0, Qt::AlignRight);

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

  void addButton(QPushButton *w)
  {
    w->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    bottom_left_layout->addWidget(w);
  }

  QPushButton *getApplyButton() { return applyButton; }

signals:

  void apply();

private Q_SLOTS:

  void toggleState()
  {
    toggle = 1 - toggle;
    if (toggle)
    {
      panel->hide();
      openClosed->setText(">");
      adjustSize();
    }
    else
    {
      panel->show();
      openClosed->setText("v");
      adjustSize();
    }

    Q_EMIT parent->embeddedWidgetSizeUpdated();
  }

private:
  NodeDataModel *parent;
  QWidget *panel;
  QPushButton *openClosed;
  QPushButton *applyButton;
  int toggle = 0;
  QHBoxLayout *bottom_left_layout;
};
