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

#pragma once

#include <iostream>
#include <vector>
#include <sstream>

#include "Renderer.h"
#include "MultiServerHandler.h"
#include "ServerRendering.h"

namespace gxy
{

class ViewerClientServer : public MultiServerHandler
{
public:
  static void init();
  bool handle(std::string line, std::string& reply);
  virtual ~ViewerClientServer(){}

  ViewerClientServer(SocketHandler *sh) : MultiServerHandler(sh)
  {
    first = true;

    datasets = Datasets::Cast(MultiServer::Get()->GetGlobal("global datasets"));
    if (! datasets)
    {
      datasets = Datasets::NewP();
      MultiServer::Get()->SetGlobal("global datasets", datasets);
    }

    renderer = Renderer::NewP();
    renderer->Commit();

    visualization = Visualization::NewP();
    visualization->Commit(datasets);

    camera = Camera::NewP();
    camera->Commit();

    rendering = ServerRendering::NewP();
    rendering->SetTheVisualization(visualization);
    rendering->SetTheCamera(camera);
    rendering->SetTheDatasets(datasets);
    rendering->Commit();

    renderingSet = RenderingSet::NewP();
    renderingSet->AddRendering(rendering);
    renderingSet->Commit();
  }

  void Commit()
  {
    renderer->Commit();
    // visualization->Commit(datasets);
    // camera->Commit();
    // rendering->Commit();
    // renderingSet->Commit();
  }

  DatasetsP        GetTheDatasets() { return datasets; }
  RendererP        GetTheRenderer() { return renderer; }
  VisualizationP   GetTheVisualization() { return visualization; }
  CameraP          GetTheCamera() { return camera; }
  ServerRenderingP GetTheRendering() { return rendering; }
  RenderingSetP    GetTheRenderingSet() { return renderingSet; }

  void render()
  {
    if (first)
    {
      renderer->Commit();
      visualization->Commit(datasets);
      rendering->Commit();
      renderingSet->Commit();
      first = false;
    }

    // camera->Commit();
    renderer->Start(renderingSet);
  }

private:
  bool first;

  DatasetsP        datasets;
  RendererP        renderer;
  VisualizationP   visualization;
  CameraP          camera;
  ServerRenderingP rendering;
  RenderingSetP    renderingSet;
};

}
