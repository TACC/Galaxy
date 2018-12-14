// ========================================================================== //
// Copyright (c) 2014-2018 The University of Texas at Austin.                 //
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

#include <iostream>
#include <vector>
#include <sstream>

using namespace std;

#include <ospray/ospray.h>

#include <string.h>

#include "rapidjson/document.h"

using namespace rapidjson;

#include "Application.h"
#include "Renderer.h"
#include "MultiServer.h"
#include "ServerRendering.h"

using namespace gxy;

#include "trackball.hpp"

float viewdistance, aov;
vec3f viewpoint, viewdirection, viewup;

float orig_viewdistance, orig_aov;
vec3f orig_viewpoint, orig_viewdirection, orig_viewup;
vec3f center;

vec3f down_viewpoint;
float down_scaling;

Trackball trackball;
float scaling;

bool ready = false;

class RenderingState
{
public:
  RenderingState(MultiServer *srvr, MultiServerSocket *skt) : srvr(srvr), skt(skt)
  {
    first = true;

    datasets = Datasets::Cast(GetTheApplication()->GetGlobal("global datasets"));
    if (! datasets)
    {
      datasets = Datasets::NewP();
      GetTheApplication()->SetGlobal("global datasets", datasets);
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

#if 0
    renderer->Commit();
    visualization->Commit(datasets);
    camera->Commit();
    rendering->Commit();
    renderingSet->Commit();
#endif
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

    camera->Commit();

    renderer->Render(renderingSet);
    std::string ok("render ok");
    skt->CSend(ok.c_str(), ok.length()+1);
  }

private:
  bool first;

  MultiServer       *srvr;
  MultiServerSocket *skt;

  DatasetsP        datasets;
  RendererP        renderer;
  VisualizationP   visualization;
  CameraP          camera;
  ServerRenderingP rendering;
  RenderingSetP    renderingSet;
};

extern "C" bool
server(MultiServer *srvr, MultiServerSocket *skt)
{
  DatasetsP theDatasets = Datasets::Cast(GetTheApplication()->GetGlobal("global datasets"));
  if (! theDatasets)
  {
    theDatasets = Datasets::NewP();
    GetTheApplication()->SetGlobal("global datasets", theDatasets);
  }

  RenderingState renderingState(srvr, skt);

  bool client_done = false;
  while (! client_done)
  {
    char *buf; int sz;
    if (! skt->CRecv(buf, sz))
      break;

    if (!strncmp(buf, "json", 4))
    {
      Document doc;
      doc.Parse(buf + 5);
  
      if (doc.HasMember("Datasets"))
      {

        renderingState.GetTheDatasets()->LoadFromJSON(doc);
        renderingState.GetTheDatasets()->Commit();
      }

      if (doc.HasMember("Visualization"))
      {
        renderingState.GetTheVisualization()->LoadFromJSON(doc["Visualization"]);
        renderingState.GetTheVisualization()->Commit(renderingState.GetTheDatasets());
      }

      if (doc.HasMember("Camera"))
      {
        renderingState.GetTheCamera()->LoadFromJSON(doc["Camera"]);
        renderingState.GetTheCamera()->Commit();
      }

      if (doc.HasMember("Renderer"))
      {
        renderingState.GetTheRenderer()->LoadStateFromDocument(doc);
        renderingState.GetTheRenderer()->Commit();
      }

      std::string ok("json ok");
      skt->CSend(ok.c_str(), ok.length()+1);
    }
    else if (! strncmp(buf, "window", 6))
    {
      stringstream ss(buf+6);
      int w, h;
      ss >> w >> h;

      renderingState.GetTheRendering()->SetTheSize(w, h);
      renderingState.GetTheRendering()->SetSocket(skt);
      renderingState.GetTheRendering()->Commit();

      std::string ok("window ok");
      skt->CSend(ok.c_str(), ok.length()+1);
    }
    else if (!strcmp(buf, "query datasets"))
    {
      vector<string> names = renderingState.GetTheDatasets()->GetDatasetNames();

      stringstream ss;
      for (vector<string>::iterator it = names.begin(); it != names.end(); it++)
      {
        KeyedDataObjectP kdop = renderingState.GetTheDatasets()->Find(*it);
        ss << *it << " " << (kdop ? kdop->GetClassName() : "huh? no class name?") << "\n";
      }

      skt->CSend(const_cast<char *>(ss.str().c_str()), ss.str().size()+1);
    }
    else if (! strcmp(buf, "render"))
    {
      renderingState.render();
    }
    else
    {
      std::cerr << "huh?\n";
      std::string ok("huh ok");
      skt->CSend(ok.c_str(), ok.length()+1);
    }

    free(buf);
  }

  return true;
}

extern "C" void
init()
{
  ospInit(0, NULL);
  Renderer::Initialize();
  ServerRendering::RegisterClass();
}
