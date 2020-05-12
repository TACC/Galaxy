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

#include <nodes/Node>
#include <nodes/NodeData>
#include <nodes/FlowScene>
#include <nodes/FlowView>

#include <QInputEvent>

#include "GxyModel.hpp"

class GxyFlowView : public QtNodes::FlowView
{
  Q_OBJECT

public:
  GxyFlowView(QtNodes::FlowScene* fs) : QtNodes::FlowView(fs)
  {
    __scene = fs;
  }

  void mousePressEvent(QMouseEvent *e) override
  {
    if (pendingModel == "")
    {
      FlowView::mousePressEvent(e);
    }
    else
    {
      auto type = __scene->registry().create(pendingModel);
      pendingModel = "";
      if (type)
      {
        auto& node = __scene->createNode(std::move(type));
        QPoint pos = e->pos();
        QPointF posView = this->mapToScene(pos);
        node.nodeGraphicsObject().setPos(posView);
        __scene->nodePlaced(node);
      }
    }
  }

  void setPendingModel(QString p)
  {
    pendingModel = p;
  }

  void addLabel()
  {
    for (QGraphicsItem * item : __scene->selectedItems())
    {
      if (auto n = qgraphicsitem_cast<QtNodes::NodeGraphicsObject*>(item))
      {
        GxyModel *model = dynamic_cast<GxyModel*>(n->node().nodeDataModel());
        model->addLabel();
      }
    }
  }

protected:
  void wheelEvent(QWheelEvent *event) override
  {
    QPoint delta = event->angleDelta();
  
    if (delta.y() == 0)
    { 
      event->ignore();
      return;
    }
  
    double const d = delta.y() / std::abs(delta.y());
  
    if (d > 0.0)
    {
      QTransform t = transform();
      if (t.m11() > 2.0)
        return;
      scale(1.01, 1.01);
    }
    else
      scale(1.0 / 1.01, 1.0 / 1.01);
  }

private:
  QtNodes::FlowScene *__scene;
  QString pendingModel = "";
};


