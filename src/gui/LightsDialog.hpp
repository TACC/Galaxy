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

#include "dtypes.h"

#include "Lights.hpp"

#include <QtCore/QObject>

#include <QtWidgets/QDialog>
#include <QtWidgets/QFrame>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QGridLayout>

#include <QtGui/QDoubleValidator>

class LightWidget : public QFrame
{
  Q_OBJECT

public:
  LightWidget(Light& l)
  {
    light = l;
    build();
  }

  LightWidget()
  {
    build();
  }

  void build()
  {
    gxy::vec3f p = light.get_point();

    QHBoxLayout *layout = new QHBoxLayout();
    layout->setSpacing(0);
    layout->setContentsMargins(2, 0, 2, 0);
    
    _x = new QLineEdit();
    _x->setFixedWidth(100);
    _x->setValidator(new QDoubleValidator());
    _x->setText(QString::number(p.x));
    layout->addWidget(_x);
    
    _y = new QLineEdit();
    _y->setFixedWidth(100);
    _y->setValidator(new QDoubleValidator());
    _y->setText(QString::number(p.y));
    layout->addWidget(_y);
    
    _z = new QLineEdit();
    _z->setFixedWidth(100);
    _z->setValidator(new QDoubleValidator());
    _z->setText(QString::number(p.z));
    layout->addWidget(_z);

    _t = new QComboBox();
    _t->addItem("infinite");
    _t->addItem("absolute");
    _t->addItem("camera relative");
    _t->setCurrentIndex(light.get_type());
    layout->addWidget(_t);

    QPushButton *x = new QPushButton("x");
    x->setAutoDefault(false);
    connect(x, SIGNAL(released()), this, SLOT(deleteLater()));
    layout->addWidget(x);

    setLayout(layout);
  }

  void get_light(Light& l)
  {
    l.set_point({ static_cast<float>(_x->text().toDouble()), 
                  static_cast<float>(_y->text().toDouble()), 
                  static_cast<float>(_z->text().toDouble()) });
    l.set_type(_t->currentIndex());
  }

private:
  QLineEdit *_x, *_y, *_z;
  QComboBox *_t;
  Light light;

};

class LightsDialog : public QDialog
{
  Q_OBJECT

public:
  LightsDialog(std::vector<Light> lights)
  {
    QVBoxLayout *layout = new QVBoxLayout();
    layout->setSpacing(0);
    layout->setContentsMargins(2, 0, 2, 0);

    QFrame *lights_frame = new QFrame();
    lights_layout = new QVBoxLayout();
    lights_layout->setSpacing(0);
    lights_layout->setContentsMargins(2, 0, 2, 0);
    lights_frame->setLayout(lights_layout);
    layout->addWidget(lights_frame);

    QFrame *buttonbox = new QFrame();
    QGridLayout *buttonbox_layout = new QGridLayout();
    buttonbox->setLayout(buttonbox_layout);

    QPushButton *ok = new QPushButton("OK");
    ok->setDefault(true);
    connect(ok, SIGNAL(released()), this, SLOT(hide()));
    buttonbox_layout->addWidget(ok, 0, 1);

    QPushButton *add = new QPushButton("add");
    add->setAutoDefault(false);
    connect(add, SIGNAL(released()), this, SLOT(add_light()));
    buttonbox_layout->addWidget(add, 0, 0);

    layout->addWidget(buttonbox);

    for (auto l : lights)
    {
      LightWidget *lw = new LightWidget(l);
      lights_layout->addWidget(lw);
    }

    setLayout(layout);
  }

  void get_lights(std::vector<Light>& lights)
  {
    lights.clear();
    for (auto i = 0; i < lights_layout->count(); i++)
    {
      LightWidget *l = (LightWidget *)(lights_layout->itemAt(i)->widget());
      Light light;
      l->get_light(light);
      lights.push_back(light);
    }
  }

public Q_SLOTS:

  void add_light()
  {
    lights_layout->addWidget(new LightWidget());
  }


private:
  QVBoxLayout *lights_layout;
};
