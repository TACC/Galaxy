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

#include "dtypes.h"

class PlaneWidget : public QFrame
{
  Q_OBJECT;

public:
  PlaneWidget(gxy::vec4f p)
  {
    plane = p;
    build();
  }

  PlaneWidget()
  {
    plane = {0.0, 0.0, 1.0, 0.0};
    build();
  }

  void build()
  {
    setFrameStyle(QFrame::Panel | QFrame::Raised);

    QHBoxLayout *layout = new QHBoxLayout();
    layout->setSpacing(0);
    layout->setContentsMargins(2, 0, 2, 0);

    // buttonbox frame and layout

    _a = new QLineEdit();
    _a->setValidator(new QDoubleValidator());
    _a->setText(QString::number(plane.x));
    _a->setFixedWidth(100);
    layout->addWidget(_a);
    connect(_a, &QLineEdit::textChanged, this, &PlaneWidget::set_a);

    _b = new QLineEdit();
    _b->setValidator(new QDoubleValidator());
    _b->setText(QString::number(plane.y));
    _b->setFixedWidth(100);
    layout->addWidget(_b);
    connect(_b, &QLineEdit::textChanged, this, &PlaneWidget::set_b);

    _c = new QLineEdit();
    _c->setValidator(new QDoubleValidator());
    _c->setText(QString::number(plane.z));
    _c->setFixedWidth(100);
    layout->addWidget(_c);
    connect(_c, &QLineEdit::textChanged, this, &PlaneWidget::set_c);

    _d = new QLineEdit();
    _d->setValidator(new QDoubleValidator());
    _d->setText(QString::number(plane.w));
    _d->setFixedWidth(100);
    layout->addWidget(_d);
    connect(_d, &QLineEdit::textChanged, this, &PlaneWidget::set_d);

    QPushButton *x = new QPushButton("x");
    x->setAutoDefault(false);
    connect(x, SIGNAL(released()), this, SLOT(deleteLater()));
    layout->addWidget(x);


    setLayout(layout);
  }

  ~PlaneWidget() {};

  gxy::vec4f get_plane() { return plane; } 

private Q_SLOTS:

  void set_a(QString const &string) { plane.x = _a->text().toDouble(); }
  void set_b(QString const &string) { plane.y = _b->text().toDouble(); }
  void set_c(QString const &string) { plane.z = _c->text().toDouble(); }
  void set_d(QString const &string) { plane.w = _d->text().toDouble(); }

private:
  gxy::vec4f plane;
  QLineEdit *_a, *_b, *_c, *_d;
};

class PlanesDialog : public QDialog
{
  Q_OBJECT;

public:

  PlanesDialog()
  {
    // outermost frame layout

    QVBoxLayout* outermost_layout = new QVBoxLayout();
    outermost_layout->setSpacing(0);
    outermost_layout->setContentsMargins(0, 0, 0, 0);

    // Frame for planes

    planes_frame = new QFrame();
    QVBoxLayout* planes_layout = new QVBoxLayout();
    planes_layout->setSpacing(0);
    planes_layout->setContentsMargins(0, 0, 0, 0);
    planes_frame->setLayout(planes_layout);
    outermost_layout->addWidget(planes_frame);

    // buttonbox frame and layout

    QFrame *buttonbox_frame = new QFrame();
    QGridLayout *buttonbox_layout = new QGridLayout();
    buttonbox_layout->setSpacing(0);
    buttonbox_layout->setContentsMargins(0, 0, 0, 0);
    buttonbox_frame->setLayout(buttonbox_layout);

    // Add buttonbox to outermost layout
    outermost_layout->addWidget(buttonbox_frame);

    QPushButton *ok = new QPushButton("OK");
    connect(ok, SIGNAL(released()), this, SLOT(hide()));
    buttonbox_layout->addWidget(ok, 0, 1);

    QPushButton *add = new QPushButton("add");
    connect(add, SIGNAL(released()), this, SLOT(add_plane()));
    buttonbox_layout->addWidget(add, 0, 0);

    ok->setDefault(true);
    setLayout(outermost_layout);
  };

  ~PlanesDialog() {};

  void set_planes(std::vector<gxy::vec4f> planes)
  {
    for (auto p : planes)
    {
      PlaneWidget *f = new PlaneWidget(p);
      ((QBoxLayout *)planes_frame->layout())->insertWidget(0, f, Qt::AlignTop);
    }
  }

  std::vector<gxy::vec4f> get_planes()
  {
    std::vector<gxy::vec4f> planes;

    for (auto i = 0; i < planes_frame->layout()->count(); i++)
    {
      PlaneWidget *p = (PlaneWidget *)(planes_frame->layout()->itemAt(i)->widget());
      planes.push_back(p->get_plane());
    }
    
    return planes;
  }

  void clear()
  {
    QLayoutItem* item;
    while ((item = planes_frame->layout()->takeAt(0)) != NULL)
    {
        delete item->widget();
        delete item;
    }
  }
      
private Q_SLOTS:

  void add_plane()
  {
    PlaneWidget *f = new PlaneWidget();
    ((QBoxLayout *)planes_frame->layout())->insertWidget(0, f, Qt::AlignTop);
  }

private:
  
  QFrame *planes_frame;
};
