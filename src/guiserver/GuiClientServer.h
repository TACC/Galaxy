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
#include <memory>
#include <sstream>

#include "rapidjson/document.h"

#include "Renderer.h"
#include "MultiServerHandler.h"
#include "GuiRendering.h"

#include "Filter.h"

namespace gxy
{

class GuiClientServer : public MultiServerHandler
{
  struct ClientWindow
  {
    ClientWindow()
    {
      visualization = Visualization::NewP();
      camera = Camera::NewP();
      rendering = GuiRendering::NewP();
      renderingSet = RenderingSet::NewP();
      renderingSet->AddRendering(rendering);
    }

    VisualizationP   visualization;
    CameraP          camera;
    GuiRenderingP    rendering;
    RenderingSetP    renderingSet;
  };
    
  
public:
  static void init();
  bool handle(std::string line, std::string& reply);
  virtual ~GuiClientServer();

  GuiClientServer(SocketHandler *sh) : MultiServerHandler(sh)
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
  }

  bool Sample(rapidjson::Document& params, string& reply);

  Filter *getFilter(std::string id)
  {
    std::shared_ptr<Filter> f = filters[id];
    return f ? f.get() : NULL;
  }

  void addFilter(std::string id, Filter *f)
  {
    filters[id] = std::shared_ptr<Filter>(f);
  }

  void removeFilter(std::string id)
  {
    filters.erase(id);
  }
    
  ClientWindow *getClientWindow(std::string id)
  {
    std::shared_ptr<ClientWindow> w = clientWindows[id];
    return w ? w.get() : NULL;
  }

  void addClientWindow(std::string id, ClientWindow *w)
  {
    clientWindows[id] = std::shared_ptr<ClientWindow>(w);
  }

  void removeClientWindow(std::string id)
  {
    clientWindows.erase(id);
  }
    
private:
  bool        first;
  RendererP   renderer;
  DatasetsP   datasets;

  std::map<std::string, std::shared_ptr<ClientWindow>> clientWindows;
  std::map<std::string, std::shared_ptr<Filter>> filters;
};
}
