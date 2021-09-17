// ========================================================================== //
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

#include <QtWidgets/QApplication>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QMenuBar>

#include <nodes/Node>
#include <nodes/NodeData>
#include <nodes/FlowScene>
#include <nodes/FlowView>
#include <nodes/ConnectionStyle>
#include <nodes/TypeConverter>
#include <nodes/DataModelRegistry>

#include "GxyFlowView.hpp"

#include "VolumeVisModel.hpp"
#include "ParticlesVisModel.hpp"
#include "PathlinesVisModel.hpp"
#include "DataSourceModel.hpp"
#include "RenderModel.hpp"
#include "MHSamplerFilter.hpp"
#include "DensitySamplerFilter.hpp"
#include "RaycastSamplerFilter.hpp"
#include "StreamTracerFilter.hpp"
#include "InterpolatorFilter.hpp"

#include "GxyConnectionMgr.hpp"

using QtNodes::DataModelRegistry;
using QtNodes::FlowScene;
using QtNodes::FlowView;
using QtNodes::ConnectionStyle;
using QtNodes::Connection;
using QtNodes::TypeConverter;
using QtNodes::TypeConverterId;
using QtNodes::NodeGraphicsObject;

extern GxyConnectionMgr *getTheGxyConnectionMgr();

static std::shared_ptr<QtNodes::DataModelRegistry>
registerDataModels()
{
  auto ret = std::make_shared<DataModelRegistry>();
  ret->registerModel<DataSourceModel>("Sources");
  ret->registerModel<MHSamplerFilter>("Filters");
  ret->registerModel<DensitySamplerFilter>("Filters");
  ret->registerModel<RaycastSamplerFilter>("Filters");
  ret->registerModel<StreamTracerFilter>("Filters");
  ret->registerModel<InterpolatorFilter>("Filters");
  ret->registerModel<VolumeVisModel>("Rendering");
  ret->registerModel<ParticlesVisModel>("Rendering");
  ret->registerModel<PathlinesVisModel>("Rendering");
  ret->registerModel<RenderModel>("Rendering");
  return ret;
}

class GxyMainWindow : public QMainWindow
{
  Q_OBJECT

public:
  GxyMainWindow() : QMainWindow()
  {
    QWidget *mainWidget = new QWidget;
    setCentralWidget(mainWidget);

    std::shared_ptr<QtNodes::DataModelRegistry> registry = registerDataModels();

    auto fileMenu   = menuBar()->addMenu("&File");
    auto loadAction = fileMenu->addAction("Load");
    auto saveAction = fileMenu->addAction("Save");
    auto debugAction = fileMenu->addAction("Debug");

    auto editMenu       = menuBar()->addMenu("&Edit");
    auto deleteAction   = editMenu->addAction("Delete");
    auto addLabelAction = editMenu->addAction("Add Label");

    connect(debugAction, SIGNAL(triggered()), this, SLOT(debug()));

    auto servermenu = menuBar()->addMenu("&Server");

    connectAction = servermenu->addAction("Connect");
    connect(connectAction, SIGNAL(triggered()), this, SLOT(connectToServer()));

    connectAsAction = servermenu->addAction("Connect As...");
    connect(connectAsAction, &QAction::triggered, getTheGxyConnectionMgr(), &GxyConnectionMgr::openConnectToServerDialog);

    disconnectAction = servermenu->addAction("Disconnect");
    disconnectAction->setEnabled(false);
    connect(disconnectAction, SIGNAL(triggered()), this, SLOT(disconnect()));

    partitioningAction = servermenu->addAction("Load Partitioning");
    partitioningAction->setEnabled(false);
    connect(partitioningAction, SIGNAL(triggered()), this, SLOT(loadPartitioning()));

    connect(getTheGxyConnectionMgr(), SIGNAL(connectionStateChanged(bool)), this, SLOT(enableConnectAction(bool)));

    flowScene = new FlowScene(registry, mainWidget);
    flowView = new GxyFlowView(flowScene);

    QVBoxLayout *l = new QVBoxLayout(mainWidget);
    l->setContentsMargins(0, 0, 0, 0);
    l->setSpacing(0);

    l->addWidget(flowView);

    QObject::connect(saveAction, &QAction::triggered, flowScene, &FlowScene::save);
    QObject::connect(loadAction, &QAction::triggered, flowScene, &FlowScene::load);
    QObject::connect(deleteAction, &QAction::triggered, flowView, &FlowView::deleteSelectedNodes);
    QObject::connect(deleteAction, &QAction::triggered, flowView, &FlowView::deleteSelectedNodes);
    QObject::connect(addLabelAction, &QAction::triggered, flowView, &GxyFlowView::addLabel);

    QMap<QString, QMenu*> menus;
    for (auto const &cat : registry->categories())
    { 
      auto item = menuBar()->addMenu(QString("&") + cat);
      menus[cat] = item;
    }

    for (auto const &assoc : registry->registeredModelsCategoryAssociation())
    {
      auto menu = menus[assoc.second];
      auto action = menu->addAction(assoc.first);
      connect(action, &QAction::triggered, [=]() {
         std::cerr << "model menu selection... " << action->text().toStdString() << "\n";
         flowView->setPendingModel(action->text());
      });
    }

    setWindowTitle("Galaxy");
    resize(1200, 900);
  }

  void 
  closeEvent(QCloseEvent *event)
  {
    QApplication::closeAllWindows();
    QMainWindow::closeEvent(event);
  }

public Q_SLOTS:
  
  void debug()
  {
  }

  void createNode(char *name)
  {
    std::cerr << "createnode " << name << "\n";
  } 

  void disconnect()
  {
    std::cerr << "DISCONNECT\n";
    getTheGxyConnectionMgr()->disconnectFromServer();
  }

  void enableConnectAction(bool b)
  {
    connectAction->setEnabled(!b);
    connectAsAction->setEnabled(!b);
    disconnectAction->setEnabled(b);
    partitioningAction->setEnabled(b);
  }

  void connectToServer()
  {
    std::cerr << "CONNECT TO SERVER\n";
    getTheGxyConnectionMgr()->connectToServer();
  }

  void loadPartitioning()
  {
    std::cerr << "load partitioning\n";
  }

private:

  QAction *connectAction;
  QAction *connectAsAction;
  QAction *partitioningAction;
  QAction *disconnectAction;
  FlowScene *flowScene;
  GxyFlowView *flowView;
};
