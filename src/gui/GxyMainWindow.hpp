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

#include <QtWidgets/QApplication>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QMenuBar>

#include <nodes/NodeData>
#include <nodes/FlowScene>
#include <nodes/FlowView>
#include <nodes/ConnectionStyle>
#include <nodes/TypeConverter>
#include <nodes/DataModelRegistry>

#include "VolumeVisModel.hpp"
#include "ParticlesVisModel.hpp"
#include "PathlinesVisModel.hpp"
#include "RenderModel.hpp"
#include "SamplerModel.hpp"
#include "StreamTracerModel.hpp"
#include "TraceDecoratorModel.hpp"
#include "InterpolatorModel.hpp"
#include "DataSourceModel.hpp"

#include "GxyConnectionMgr.hpp"

using QtNodes::DataModelRegistry;
using QtNodes::FlowScene;
using QtNodes::FlowView;
using QtNodes::ConnectionStyle;
using QtNodes::TypeConverter;
using QtNodes::TypeConverterId;

extern GxyConnectionMgr *getTheGxyConnectionMgr();

static std::shared_ptr<QtNodes::DataModelRegistry>
registerDataModels()
{
  auto ret = std::make_shared<DataModelRegistry>();
  ret->registerModel<DataSourceModel>("Data Access");
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

class GxyMainWindow : QMainWindow
{
  Q_OBJECT

public:
  GxyMainWindow() : QMainWindow()
  {
    QWidget *mainWidget = new QWidget;
    setCentralWidget(mainWidget);

    flowScene = new FlowScene(registerDataModels(), mainWidget);
    flowView = new FlowView(flowScene);

    auto fileMenu   = menuBar()->addMenu("&File");
    auto loadAction = fileMenu->addAction("Load");
    auto saveAction = fileMenu->addAction("Save");

    auto editMenu     = menuBar()->addMenu("&Edit");
   auto deleteAction = editMenu->addAction("Delete");

    auto servermenu = menuBar()->addMenu("&Server");

    connectAction = servermenu->addAction("Connect...");
    connect(connectAction, &QAction::triggered, getTheGxyConnectionMgr(), &GxyConnectionMgr::openConnectToServerDialog);

    disconnectAction = servermenu->addAction("Disconnect");
    disconnectAction->setEnabled(false);
    connect(disconnectAction, SIGNAL(triggered()), this, SLOT(disconnect()));

    connect(getTheGxyConnectionMgr(), SIGNAL(connectionStateChanged(bool)), this, SLOT(enableConnectAction(bool)));

    QVBoxLayout *l = new QVBoxLayout(mainWidget);
    l->setContentsMargins(0, 0, 0, 0);
    l->setSpacing(0);

    auto scene = new FlowScene(registerDataModels(), mainWidget);

    auto flowView = new FlowView(scene);
    l->addWidget(flowView);

    QObject::connect(saveAction, &QAction::triggered, scene, &FlowScene::save);
    QObject::connect(loadAction, &QAction::triggered, scene, &FlowScene::load);
    QObject::connect(deleteAction, &QAction::triggered, flowView, &FlowView::deleteSelectedNodes);

    setWindowTitle("Galaxy");
    resize(800, 600);
    showNormal();
  }

public Q_SLOTS:

  void disconnect()
  {
    getTheGxyConnectionMgr()->Disconnect();
    enableConnectAction(false);
  }

  void enableConnectAction(bool b)
  {
    connectAction->setEnabled(!b);
    disconnectAction->setEnabled(b);
  }

private:

  QAction *connectAction;
  QAction *disconnectAction;
  FlowScene *flowScene;
  FlowView *flowView;
};
