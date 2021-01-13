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
#include <memory>
#include <sstream>

#include "rapidjson/document.h"

#include "Renderer.h"
#include "MultiServerHandler.h"
#include "Datasets.h"
#include "GuiRendering.h"

#include "GalaxyObject.h"

#include "Filter.h"

namespace gxy
{

class GuiClientServer : public MultiServerHandler
{
  struct ClientWindow
  {
    ClientWindow(string id)
    {
      visualization = Visualization::NewDistributed();
      camera = Camera::NewDistributed();

      rendering = GuiRendering::NewDistributed();
      rendering->SetId(id);

      renderingSet = RenderingSet::NewDistributed();
      renderingSet->AddRendering(rendering);

      frame = 0;
    }

    VisualizationDPtr   visualization;
    CameraDPtr          camera;
    GuiRenderingDPtr    rendering;
    RenderingSetDPtr    renderingSet;
    DatasetsDPtr        datasets;

    int frame;
  };
    
  
public:
  static void init();
  bool handle(std::string line, std::string& reply) override;
  virtual ~GuiClientServer();

  GuiClientServer(SocketHandler *sh) : MultiServerHandler(sh)
  {
    first = true;

    globals = Datasets::DCast(MultiServer::Get()->GetGlobal("global datasets"));
    if (! globals)
    {
      globals = Datasets::NewDistributed();
      MultiServer::Get()->SetGlobal("global datasets", globals);
    }

    Observe(globals);

    temporaries = Datasets::NewDistributed();

    renderer = Renderer::NewDistributed();
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

  virtual void Notify(GalaxyObject* o, ObserverEvent id, void *cargo) override;

private:
  bool        first;
  RendererDPtr   renderer;

  std::vector<std::string> watched_datasets;

  std::map<std::string, std::shared_ptr<ClientWindow>> clientWindows;
  std::map<std::string, std::shared_ptr<Filter>> filters;
  
  DatasetsDPtr temporaries;
  DatasetsDPtr globals;
};
}
