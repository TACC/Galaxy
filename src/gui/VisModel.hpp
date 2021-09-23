// ========================================================================== //
//                                                                            //
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

#pragma once

#include "dtypes.h"

#include <vector>
#include <string>

#include <QtCore/QObject>
#include <QPixmap>

#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QSlider>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QFileDialog>

#include <QtGui/QDoubleValidator>

#include "GxyModel.hpp"
#include "CMapDialog.hpp"

using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeDataModel;
using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeValidationState;

#include "GxyData.hpp"
#include "PlanesDialog.hpp"
#include "ScalarsDialog.hpp"

#include <QJsonArray>

#include "Vis.hpp"

class VisModel : public GxyModel
{
  Q_OBJECT

public:
  VisModel();
  ~VisModel() {}

  unsigned int nPorts(PortType portType) const override;

  NodeValidationState validationState() const override;

  QString validationMessage() const override;

  QString caption() const override { return QStringLiteral("Vis"); }

  QString name() const override { return QStringLiteral("Vis"); }

  QJsonObject save() const override;

  void setInData(std::shared_ptr<QtNodes::NodeData> data, QtNodes::PortIndex portIndex) override;

  virtual void loadInputDrivenWidgets(std::shared_ptr<GxyPacket> o) override;
  virtual void loadParameterWidgets() override;

  virtual void loadOutput(std::shared_ptr<GxyData> o) const override;

  bool isValid() override;

public Q_SLOTS:

  void onApply() override;

  void onDataRangeReset();

  void onCmapMinTextChanged();
  void onCmapMaxTextChanged();

  void onCmapMinSliderChanged(int v);
  void onCmapMaxSliderChanged(int v);

  void openCmapSelectorDialog()
  {
    CMapDialog d(current_colormap);
    connect(&d, SIGNAL(cmap_selected(std::string)), this, SLOT(cmap_selected(std::string)));
    d.exec();
    enableIfValid();
  }

  void cmap_selected(std::string s)
  {
    load_cmap(s);
  }

protected:
  // std::shared_ptr<Vis> output;

  void load_cmap(std::string s)
  {
    if (s.substr(0, strlen("GALAXY_ROOT")) == "GALAXY_ROOT")
      s = std::string(getenv("GALAXY_ROOT")) + s.substr(strlen("GALAXY_ROOT"));
    else if (s.substr(0, strlen("HOME")) == "HOME")
      s = std::string(getenv("HOME")) + s.substr(strlen("HOME"));

    int n = s.find_last_of(".");
    std::string root = (n != s.npos) ? s.substr(0, n) : s;

    n = root.find_last_of("/");
    std::string name = (n != root.npos) ? root.substr(n+1) : root;

    current_colormap.assign((root + ".json").c_str());

    cmap_text_widget->clear();
    cmap_text_widget->insert(name.c_str());

    QPixmap *pm = new QPixmap((root + ".png").c_str());
    cmap_label_widget->setPixmap(pm->scaled(cmap_label_widget->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
  }
  
  

private:
  bool      data_range_set = false;
  float     data_minimum, data_maximum;
  QSlider   *cmap_range_min_slider = NULL;
  QSlider   *cmap_range_max_slider = NULL;
  QLineEdit *cmap_text_widget = NULL;
  QLabel    *cmap_label_widget = NULL;
  QLineEdit *cmap_range_min = NULL;
  QLineEdit *cmap_range_max = NULL;

  std::string current_colormap;
};
