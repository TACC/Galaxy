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

#include "Renderer.h"
#include "MultiServer.h"
#include "MultiServerHandler.h"
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
  RenderingState(MultiServerHandler *handler) : handler(handler)
  {
std::cerr << "RenderingState ctor\n";
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

  ~RenderingState()
  {
    // Delete(datasets);
    // Delete(visualization);
    // Delete(camera);
    // Delete(rendering);
    // Delete(renderingSet);
    // Delete(renderer);
  }

  void Commit()
  {
    std::cerr << "COMMIT ================\n";
    renderer->Commit();
    visualization->Commit(datasets);
    camera->Commit();
    rendering->Commit();
    renderingSet->Commit();
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
std::cerr << "RENDER first\n";
      renderer->Commit();
      visualization->Commit(datasets);
      rendering->Commit();
      renderingSet->Commit();
      first = false;
    }

    camera->Commit();

    renderer->Render(renderingSet);
    std::string ok("render ok");
    handler->CSend(ok.c_str(), ok.length()+1);
  }

private:
  bool first;

  MultiServerHandler *handler;

  DatasetsP        datasets;
  RendererP        renderer;
  VisualizationP   visualization;
  CameraP          camera;
  ServerRenderingP rendering;
  RenderingSetP    renderingSet;
};

void brk() { std::cerr << "server break\n"; }

extern "C" bool
server(MultiServerHandler *handler)
{
  DatasetsP theDatasets = Datasets::Cast(MultiServer::Get()->GetGlobal("global datasets"));
  if (! theDatasets)
  {
    theDatasets = Datasets::NewP();
    MultiServer::Get()->SetGlobal("global datasets", theDatasets);
  }

  RenderingState renderingState(handler);

  bool client_done = false;
  while (! client_done)
  {
    std::string line;
    if (! handler->CRecv(line))
      break;

    std::stringstream ss(line);
    std::string cmd;

    ss >> cmd;

    if (cmd == "sbreak")
      brk();

		else if (cmd == "permute_pixels")
		{
			std::string onoff;
			ss >> onoff;
			if (onoff == "on")
				renderingState.GetTheRenderer()->SetPermutePixels(true);
			else if (onoff == "off")
				renderingState.GetTheRenderer()->SetPermutePixels(false);
			else
				std::cerr << "invalid permute_pixels: arg must be on or off\n";
		}

		else if (cmd == "max_rays_per_packet")
		{
			int n;
			ss >> n;
			renderingState.GetTheRenderer()->SetMaxRayListSize(n);
		}

    else if (cmd == "commit")
      renderingState.Commit();

    else if (cmd == "json")
    {
      Document doc;

      string json, tmp;
      while (std::getline(ss, tmp))
        json = json + tmp;

      doc.Parse(json.c_str());
  
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
    }
    else if (cmd == "window")
    {
      int w, h;
      ss >> w >> h;

      renderingState.GetTheRendering()->SetTheSize(w, h);
      renderingState.GetTheRendering()->SetHandler(handler);
      renderingState.GetTheRendering()->Commit();
    }
    else if (cmd == "render")
    {
      renderingState.render();
    }
    else
    {
      std::string args;
      std::getline(ss, args);
      handler->handle(cmd, args);
    }
      
    std::string ok("ok");
    handler->CSend(ok.c_str(), ok.length()+1);
  }

  // ospShutdown();

  return true;
}

extern "C" void
init()
{
  Renderer::Initialize();
  ServerRendering::RegisterClass();
}
