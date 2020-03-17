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
#include "dtypes.h"

#include "Camera.hpp"

#include <QtCore/QObject>

#include <QtWidgets/QDialog>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>

#include <QtGui/QDoubleValidator>
#include <QtGui/QIntValidator>

#define FGridCell(te, v, r, c)                                  \
{                                                               \
  te = new QLineEdit();                                         \
  te->setText(QString::number(v));                              \
  te->setValidator(new QDoubleValidator());                     \
  grid_box_layout->addWidget(te, r, c);                         \
} 

class CameraDialog : public QDialog
{
  Q_OBJECT

public:
  CameraDialog(Camera& c)
  {
    camera = c;

    QVBoxLayout *outer_layout = new QVBoxLayout();
    outer_layout->setSpacing(0);
    outer_layout->setContentsMargins(2, 0, 2, 0);

    QWidget *grid_box = new QWidget();
    outer_layout->addWidget(grid_box);

    QGridLayout *grid_box_layout = new QGridLayout();
    grid_box_layout->setSpacing(0);
    grid_box_layout->setContentsMargins(2, 0, 2, 0);
    grid_box->setLayout(grid_box_layout);

    grid_box_layout->addWidget(new QLabel("viewpoint"), 0, 0);
    FGridCell(_vp_x, camera.point.x, 0, 1)
    FGridCell(_vp_y, camera.point.y, 0, 2)
    FGridCell(_vp_z, camera.point.z, 0, 3)

    grid_box_layout->addWidget(new QLabel("view direction"), 1, 0);
    FGridCell(_vd_x, camera.direction.x, 1, 1)
    FGridCell(_vd_y, camera.direction.y, 1, 2)
    FGridCell(_vd_z, camera.direction.z, 1, 3)

    grid_box_layout->addWidget(new QLabel("view up"), 2, 0);
    FGridCell(_vu_x, camera.up.x, 2, 1)
    FGridCell(_vu_y, camera.up.y, 2, 2)
    FGridCell(_vu_z, camera.up.z, 2, 3)

    grid_box_layout->addWidget(new QLabel("angle of view"), 3, 0);
    FGridCell(_aov, camera.aov, 3, 1);

    QWidget *size_frame = new QWidget();
    QHBoxLayout *size_layout = new QHBoxLayout();
    size_layout->setSpacing(0);
    size_layout->setContentsMargins(2, 0, 2, 0);
    size_frame->setLayout(size_layout);

    size_layout->addWidget(new QLabel("Image size     "));

    _w = new QLineEdit();
    _w->setText(QString::number(camera.size.x));    
    _w->setValidator(new QIntValidator());  
    size_layout->addWidget(_w);

    size_layout->addWidget(new QLabel("x"));
    
    _h = new QLineEdit();
    _h->setText(QString::number(camera.size.y));
    _h->setValidator(new QIntValidator());
    size_layout->addWidget(_h);

    outer_layout->addWidget(size_frame);

    QWidget *button_box = new QWidget();
    QGridLayout *button_box_layout = new QGridLayout();
    button_box->setLayout(button_box_layout);

    QPushButton *reset = new QPushButton("Reset");
    reset->setAutoDefault(false);
    connect(reset, SIGNAL(released()), this, SLOT(reset()));
    button_box_layout->addWidget(reset, 0, 2);

    QPushButton *ok = new QPushButton("OK");
    ok->setDefault(true);
    connect(ok, SIGNAL(released()), this, SLOT(hide()));
    button_box_layout->addWidget(ok, 0, 3);

    outer_layout->addWidget(button_box);

    setLayout(outer_layout);
  }

  ~CameraDialog() {}

  void get_camera(Camera& c)
  {
    c.setPoint(_vp_x->text().toDouble(),
               _vp_y->text().toDouble(),
               _vp_z->text().toDouble());
    c.setDirection(_vd_x->text().toDouble(),
               _vd_y->text().toDouble(),
               _vd_z->text().toDouble());
    c.setUp(_vu_x->text().toDouble(),
               _vu_y->text().toDouble(),
               _vu_z->text().toDouble());
    c.setSize(_w->text().toInt(),
              _h->text().toInt());
    c.setAOV(_aov->text().toDouble());
    gxy::vec3f p = c.getPoint();
  }


public Q_SLOTS:

  void reset()
  {
    _vp_x->setText(QString::number(camera.point.x));
    _vp_y->setText(QString::number(camera.point.y));
    _vp_z->setText(QString::number(camera.point.z));

    _vd_x->setText(QString::number(camera.direction.x));
    _vd_y->setText(QString::number(camera.direction.y));
    _vd_z->setText(QString::number(camera.direction.z));

    _vu_x->setText(QString::number(camera.up.x));
    _vu_y->setText(QString::number(camera.up.y));
    _vu_z->setText(QString::number(camera.up.z));

    _w->setText(QString::number(camera.size.x));
    _h->setText(QString::number(camera.size.y));
    _vu_z->setText(QString::number(camera.up.z));

    _aov->setText(QString::number(camera.aov));
  }



private:

  Camera camera;

  QLineEdit *_w, *_h;

  QLineEdit *_vp_x, *_vp_y, *_vp_z;
  QLineEdit *_vd_x, *_vd_y, *_vd_z;
  QLineEdit *_vu_x, *_vu_y, *_vu_z;
  QLineEdit *_aov;
};
