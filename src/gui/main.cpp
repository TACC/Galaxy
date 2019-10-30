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

#include <nodes/NodeData>
#include <nodes/FlowScene>
#include <nodes/FlowView>
#include <nodes/ConnectionStyle>
#include <nodes/TypeConverter>

#include <QtWidgets/QApplication>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QMenuBar>

#include <nodes/DataModelRegistry>

#include "VolumeVisModel.hpp"
#include "ParticlesVisModel.hpp"
#include "PathlinesVisModel.hpp"
#include "ImporterModel.hpp"
#include "RenderModel.hpp"
#include "SamplerModel.hpp"
#include "StreamTracerModel.hpp"
#include "TraceDecoratorModel.hpp"
#include "InterpolatorModel.hpp"

using QtNodes::DataModelRegistry;
using QtNodes::FlowScene;
using QtNodes::FlowView;
using QtNodes::ConnectionStyle;
using QtNodes::TypeConverter;
using QtNodes::TypeConverterId;

static std::shared_ptr<DataModelRegistry>
registerDataModels()
{
  auto ret = std::make_shared<DataModelRegistry>();
  ret->registerModel<ImporterModel>("Data Access");
  ret->registerModel<SamplerModel>("Filters");
  ret->registerModel<StreamTracerModel>("Filters");
  ret->registerModel<TraceDecoratorModel>("Filters");
  ret->registerModel<InterpolatorModel>("Filters");
  ret->registerModel<VolumeVisModel>("Visualizations");
  ret->registerModel<ParticlesVisModel>("Visualizations");
  ret->registerModel<PathlinesVisModel>("Visualizations");
  ret->registerModel<RenderModel>("Rendering");
  return ret;
}

static
void
setStyle()
{
  ConnectionStyle::setConnectionStyle(
  R"(
  {
    "ConnectionStyle": {
      "ConstructionColor": "gray",
      "NormalColor": "black",
      "SelectedColor": "gray",
      "SelectedHaloColor": "deepskyblue",
      "HoveredColor": "deepskyblue",

      "LineWidth": 3.0,
      "ConstructionLineWidth": 2.0,
      "PointDiameter": 10.0,

      "UseDataDefinedColors": true
    }
  }
  )");
}

#if 1

int
main(int argc, char *argv[])
{
  QApplication app(argc, argv);

  setStyle();

  QMainWindow mainWindow;

  QWidget *mainWidget = new QWidget;
  mainWindow.setCentralWidget(mainWidget);

  auto menuBar    = mainWindow.menuBar();
  auto fileMenu   = menuBar->addMenu("&File");
  auto loadAction = fileMenu->addAction("Load");
  auto saveAction = fileMenu->addAction("Save");

  auto editMenu     = menuBar->addMenu("&Edit");
  auto deleteAction = editMenu->addAction("Delete");

  QVBoxLayout *l = new QVBoxLayout(mainWidget);
  l->setContentsMargins(0, 0, 0, 0);
  l->setSpacing(0);

  auto scene = new FlowScene(registerDataModels(), mainWidget);

  auto flowView = new FlowView(scene);
  l->addWidget(flowView);

  QObject::connect(saveAction, &QAction::triggered, scene, &FlowScene::save);
  QObject::connect(loadAction, &QAction::triggered, scene, &FlowScene::load);
  QObject::connect(deleteAction, &QAction::triggered, flowView, &FlowView::deleteSelectedNodes);

  mainWindow.setWindowTitle("Galaxy");
  mainWindow.resize(800, 600);
  mainWindow.showNormal();

  app.exec();

}

#else

int
main(int argc, char *argv[])
{
  QApplication app(argc, argv);

  setStyle();

  QWidget mainWidget;

  auto menuBar    = new QMenuBar();
  auto saveAction = menuBar->addAction("Save..");
  auto loadAction = menuBar->addAction("Load..");

  QVBoxLayout *l = new QVBoxLayout(&mainWidget);

  l->addWidget(menuBar);
  auto scene = new FlowScene(registerDataModels(), &mainWidget);
  l->addWidget(new FlowView(scene));
  l->setContentsMargins(0, 0, 0, 0);
  l->setSpacing(0);

  QObject::connect(saveAction, &QAction::triggered, scene, &FlowScene::save);

  QObject::connect(loadAction, &QAction::triggered, scene, &FlowScene::load);

  mainWidget.setWindowTitle("Galaxy");
  mainWidget.resize(800, 600);

  mainWidget.showNormal();

  return app.exec();
}

#endif
