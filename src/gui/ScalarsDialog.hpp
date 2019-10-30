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


#include <vector>

#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QFrame>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>

#include <QtGui/QDoubleValidator>

class Scalar : public QFrame
{
  Q_OBJECT;

public:
  Scalar(float& s) { scalar = s; }
  Scalar() { scalar = 0; }

  Scalar *build()
  {
    setFrameStyle(QFrame::Panel | QFrame::Raised);

    QHBoxLayout *layout = new QHBoxLayout();
    layout->setSpacing(0);
    layout->setContentsMargins(2, 0, 2, 0);

    _scalar = new QLineEdit();
    _scalar->setValidator(new QDoubleValidator());
    _scalar->setText(QString::number(scalar));
    _scalar->setFixedWidth(100);
    layout->addWidget(_scalar);

    QPushButton *x = new QPushButton("x");
    connect(x, SIGNAL(released()), this, SLOT(deleteLater()));
    layout->addWidget(x);

    setLayout(layout);
    return this;
  }

  ~Scalar() {}

  float get_scalar() { return _scalar->text().toDouble(); }

private:
  QLineEdit *_scalar;
  float scalar;
};

class ScalarsDialog : public QDialog
{
  Q_OBJECT;

public:

  ScalarsDialog(std::vector<float> scalars)
  {
    // outermost frame layout

    QVBoxLayout *outermost_layout = new QVBoxLayout();
    outermost_layout->setSpacing(0);
    outermost_layout->setContentsMargins(0, 0, 0, 0);

    // Frame for scalars

    QFrame *scalars_frame = new QFrame();
    scalars_layout = new QVBoxLayout();
    scalars_layout->setSpacing(0);
    scalars_layout->setContentsMargins(0, 0, 0, 0);
    scalars_frame->setLayout(scalars_layout);
    outermost_layout->addWidget(scalars_frame);

    // buttonbox frame and layout

    QFrame *buttonbox_frame = new QFrame();
    QGridLayout *buttonbox_layout = new QGridLayout();
    buttonbox_layout->setSpacing(0);
    buttonbox_layout->setContentsMargins(0, 0, 0, 0);
    buttonbox_frame->setLayout(buttonbox_layout);

    QPushButton *b = new QPushButton("OK");
    connect(b, SIGNAL(released()), this, SLOT(hide()));
    buttonbox_layout->addWidget(b, 0, 1);

    b = new QPushButton("add");
    connect(b, SIGNAL(released()), this, SLOT(add_scalar()));
    buttonbox_layout->addWidget(b, 0, 0);

    // Add buttonbox to outermost layout
    outermost_layout->addWidget(buttonbox_frame);

    for (auto p : scalars)
    {
      Scalar *f = new Scalar(p);
      scalars_layout->addWidget(f->build(), 0, Qt::AlignTop);
    }

    setLayout(outermost_layout);
  };

  ~ScalarsDialog() {};

  std::vector<float> get_scalars()
  {
    std::vector<float> scalars;

    for (auto i = 0; i < scalars_layout->count(); i++)
    {
      Scalar *s = (Scalar *)(scalars_layout->itemAt(i)->widget());
      scalars.push_back(s->get_scalar());
    }
    
    return scalars;
  }
      
private Q_SLOTS:

  void add_scalar()
  {
    Scalar *f = new Scalar();
    scalars_layout->addWidget(f->build(), 0, Qt::AlignTop);
  }

private:
  QVBoxLayout *scalars_layout;
};
