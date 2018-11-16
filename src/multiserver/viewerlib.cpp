#include <iostream>
#include <vector>
#include <sstream>

using namespace std;

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
  }

  DatasetsP        GetTheDatasets() { return datasets; }
  RendererP        GetTheRenderer() { return renderer; }
  VisualizationP   GetTheVisualization() { return visualization; }
  CameraP          GetTheCamera() { return camera; }
  ServerRenderingP GetTheRendering() { return rendering; }
  RenderingSetP    GetTheRenderingSet() { return renderingSet; }

  void render()
  {
#if 0
    renderer->Commit();
    visualization->Commit(datasets);
    camera->Commit();
    rendering->Commit();
    renderingSet->Commit();
#endif

    renderer->Render(renderingSet);
    std::string ok("render ok");
    skt->CSend(ok.c_str(), ok.length()+1);
  }

private:
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
    {
      cerr << "receive failed\n";
      break;
    }

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

  cerr << "worker exits\n";
  return true;
}

extern "C" void
init()
{
  ServerRendering::RegisterClass();
}
