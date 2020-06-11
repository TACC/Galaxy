// ========================================================================== //
//                                                                            //
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

#include <iostream>
#include <string>
#include <sstream>

using namespace std;

#include <string.h>

#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"

using namespace rapidjson;

#include "GuiClientServer.h"
#include "MHSampler.hpp"
#include "DensitySampler.hpp"
#include "RaycastSampler.hpp"
#include "StreamTracer.hpp"

using namespace gxy;

namespace gxy
{

extern "C" void
init()
{
  GuiClientServer::init();
  MHSampler::init();
  DensitySampler::init();
  RaycastSampler::init();
}

extern "C" GuiClientServer *
new_handler(SocketHandler *sh)
{
  return new GuiClientServer(sh);
}

void
GuiClientServer::init()
{
  Renderer::Initialize();
  Sampler::Initialize();
  GuiRendering::RegisterClass();
  StreamTracer::Register();
}

GuiClientServer::~GuiClientServer()
{
}

static std::string 
DocumentToString(rapidjson::Document& doc)
{
  rapidjson::StringBuffer strbuf(0, 65536);
  rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
  doc.Accept(writer);
  return strbuf.GetString();
}

void
GuiClientServer::Notify(GalaxyObject *w, ObserverEvent id, void *cargo)
{
  switch(id)
  { 
    case ObserverEvent::Updated:
    { 
      if (Datasets::IsA(w))
      {
        std::cerr << "GuiClientServer has been notified that the datasets object has updated\n";
        Datasets::DatasetsUpdate *update = (Datasets::DatasetsUpdate *)cargo;

        if (! update)
        {
          std::cerr << "Null cargo?\n";
          return;
        }
        else if (update->typ == Datasets::Added)
        {
          std::cerr << "Added " << update->name << "\n";

          KeyedDataObjectP kdop = temporaries->Find(update->name);
          if (! kdop)
            kdop = globals->Find(update->name);

          if (! kdop)
            std::cerr << "Couldn't find " << update->name << " in either global or temporary datasets\n";
          else
            Observe(kdop);

          Document replyDoc;
          replyDoc.SetObject();
          Document::AllocatorType& alloc = replyDoc.GetAllocator();
          replyDoc.AddMember("action", rapidjson::Value().SetString("added"), alloc);
          replyDoc.AddMember("argument", rapidjson::Value().SetString(update->name.data(), update->name.size(), alloc), alloc);
          std::string reply = DocumentToString(replyDoc);

          getTheSocketHandler()->ESend(reply);
          return;
        }
        else
        {
          std::cerr << "Unknown update type\n";
          return;
        }
      }
      else if (KeyedDataObject::IsA(w))
      {
        if (! globals)
        {
          std::cerr << "Got an update from a KeyedDataObject but there's no global datasets object\n";
          return;
        }

        std::string name;
        name = temporaries->Find((KeyedDataObject*)w);
        if (name.size() == 0)
          name = globals->Find((KeyedDataObject*)w);

        if (name.size() > 0)
        {
          Document replyDoc;
          replyDoc.SetObject();
          Document::AllocatorType& alloc = replyDoc.GetAllocator();
          replyDoc.AddMember("action", rapidjson::Value().SetString("modified"), alloc);
          replyDoc.AddMember("argument", rapidjson::Value().SetString(name.data(), name.size(), alloc), alloc);
          std::string reply = DocumentToString(replyDoc);

          getTheSocketHandler()->ESend(reply);
        }
        else
          std::cerr << "Update from unknown object\n";

        return;
      }
      else
      {
        std::cerr << "Update came from a " << w->GetClassName() << " object\n";
        return;
      }
    }
      
    default:
      super::Notify(w, id, cargo);
      return;
  }
}

bool
GuiClientServer::Sample(Document& params, std::string& reply)
{
  reply = "OK";
  return true;
}

#define HANDLED_BUT_ERROR_RETURN(msg)                                                           \
{                                                                                               \
  std::string s = "error";                                                                      \
  replyDoc.AddMember("status", Value().SetString(s.c_str(), s.length(), alloc), alloc);         \
  s = msg;                                                                                      \
  replyDoc.AddMember("error message", Value().SetString(s.c_str(), s.length(), alloc), alloc);  \
  reply = DocumentToString(replyDoc);                                                           \
  return true;                                                                                  \
}

#define HANDLED_OK                                                                              \
{                                                                                               \
  std::string s = "ok";                                                                         \
  replyDoc.AddMember("status", Value().SetString(s.c_str(), s.length(), alloc), alloc);         \
  reply = DocumentToString(replyDoc);                                                           \
  return true;                                                                                  \
}

bool
GuiClientServer::handle(string line, string& reply)
{
  Document replyDoc;
  replyDoc.SetObject();

  // std::cerr << "GUI Handle: " << line << "\n";

  Document::AllocatorType& alloc = replyDoc.GetAllocator();

  Document doc;
  if (doc.Parse<0>(line.c_str()).HasParseError())
    HANDLED_BUT_ERROR_RETURN("error parsing json command");

  string cmd = doc["cmd"].GetString();

  if (cmd == "gui::import")
  {
    if (! globals->LoadFromJSON(doc))
      HANDLED_BUT_ERROR_RETURN("import datasets: error in LoadFromJson");

    globals->Commit();

    HANDLED_OK;
  }
  else if (cmd == "gui::observe")
  {
    std::string name = doc["old"].GetString();
    if (name != "none")
    {
      std::cerr << "Unobserve: " << name << "\n";
      KeyedDataObjectP o = globals->Find(name);
      if (o)
        Unobserve(o);
    }

    name = doc["new"].GetString();
    if (name != "none")
    {
      std::cerr << "Observe: " << name << "\n";
      KeyedDataObjectP o = globals->Find(name);
      if (o)
        Observe(o);
    }

    HANDLED_OK;
  }
  else if (cmd == "gui::list")
  {
    Value array(kArrayType);

    vector<string> names = globals->GetDatasetNames();
    for (vector<string>::iterator it = names.begin(); it != names.end(); ++it)
    {
      std::string name(*it);
      KeyedDataObjectP kdop = globals->Find(name);

      auto i = watched_datasets.begin();
      while (i != watched_datasets.end() && *i != name) i++;

      int type;
      if (kdop->getclass() ==  Volume::ClassType) type = 0;
      else if (kdop->getclass() ==  Triangles::ClassType) type = 1;
      else if (kdop->getclass() ==  Particles::ClassType) type = 2;
      else if (kdop->getclass() ==  PathLines::ClassType) type = 3;
      else type = -1;

      int ncomp;
      if (type == 0)
      {
        VolumeP v = Volume::Cast(kdop);
        ncomp = v->get_number_of_components();
      }
      else
        ncomp = -1;

      float m, M;
      kdop->get_global_minmax(m, M);

      int key = kdop->getkey();

      rapidjson::Value dset(rapidjson::kObjectType);

      dset.AddMember("name", Value().SetString(it->c_str(), it->length(), alloc), alloc);
      dset.AddMember("type", type, alloc);
      dset.AddMember("key", key, alloc);
      dset.AddMember("ncomp", ncomp, alloc);
      dset.AddMember("min", m, alloc);
      dset.AddMember("max", M, alloc);

      Box *box = kdop->get_global_box();

      rapidjson::Value boxv(rapidjson::kArrayType);
      boxv.PushBack(rapidjson::Value().SetDouble(box->get_min()[0]), alloc);
      boxv.PushBack(rapidjson::Value().SetDouble(box->get_max()[0]), alloc);
      boxv.PushBack(rapidjson::Value().SetDouble(box->get_min()[1]), alloc);
      boxv.PushBack(rapidjson::Value().SetDouble(box->get_max()[1]), alloc);
      boxv.PushBack(rapidjson::Value().SetDouble(box->get_min()[2]), alloc);
      boxv.PushBack(rapidjson::Value().SetDouble(box->get_max()[2]), alloc);
      dset.AddMember("box", boxv, alloc);

      array.PushBack(dset, alloc);
    }

    replyDoc.AddMember("Datasets", array, alloc);

    std::string s = "ok";
    replyDoc.AddMember("status", Value().SetString(s.c_str(), s.length(), alloc), alloc);        
    reply = DocumentToString(replyDoc);                                                         
    // std::cerr << "REPLY: " << reply << "\n";
    return true; 

    // HANDLED_OK;
  }
  else if (cmd == "gui::initWindow")
  {
    string id  = doc["id"].GetString();
    ClientWindow *clientWindow = getClientWindow(id);
    if (clientWindow)
      HANDLED_BUT_ERROR_RETURN("initWindow: window already initialized")

    addClientWindow(id, new ClientWindow(id));

    HANDLED_OK;
  }
  else if (cmd == "gui::visualization")
  {
    string id  = doc["id"].GetString();
    ClientWindow *clientWindow = getClientWindow(id);
    if (! clientWindow)
      HANDLED_BUT_ERROR_RETURN("initWindow: window has not been initialized");

    clientWindow->visualization  = Visualization::NewP();
    clientWindow->datasets       = Datasets::NewP();

    if (! clientWindow->visualization->LoadFromJSON(doc["Visualization"]))
      HANDLED_BUT_ERROR_RETURN("visualization: error in LoadFromJson");

    for (int i = 0; i < clientWindow->visualization->GetNumberOfVis(); i++)
    {
      auto v = clientWindow->visualization->GetVis(i);

      KeyedDataObjectP kdop = temporaries->Find(v->GetName());
      if (! kdop)
        kdop = globals->Find(v->GetName());

      if (! kdop)

      {
        std::cerr << "gui::render - can't find data referenced by a vis (" << v->GetName() << ")\n";
        exit(1);
      }

      v->SetTheData(kdop);

      clientWindow->datasets->Insert(v->GetName(), kdop);
    }

    clientWindow->datasets->Commit();
    clientWindow->visualization->Commit(clientWindow->datasets);

    HANDLED_OK;
  }
  else if (cmd == "gui::camera")
  {
    string id  = doc["id"].GetString();

    ClientWindow *clientWindow = getClientWindow(id);
    if (! clientWindow)
      HANDLED_BUT_ERROR_RETURN("camera: window has not been initialized");

    if (! clientWindow->camera->LoadFromJSON(doc["Camera"]))
      HANDLED_BUT_ERROR_RETURN("camera: error in LoadFromJson");

    clientWindow->camera->Commit();

    HANDLED_OK;
  }
  else if (cmd == "gui::render")
  {
    string id  = doc["id"].GetString();

    ClientWindow *clientWindow = getClientWindow(id);
    if (! clientWindow)
      HANDLED_BUT_ERROR_RETURN("render: window has not been initialized");
    
    GuiRenderingP rendering = GuiRendering::NewP();
    clientWindow->renderingSet = RenderingSet::NewP();
    clientWindow->renderingSet->SetRenderFrame(clientWindow->frame++);

    CameraP camera = clientWindow->camera;
    rendering->SetTheSize(camera->get_width(), camera->get_height());
    rendering->SetTheCamera(camera);

    rendering->SetHandler(this);
    rendering->SetTheVisualization(clientWindow->visualization);
    rendering->SetTheDatasets(clientWindow->datasets);
    rendering->SetId(id);
    rendering->Commit();

    clientWindow->renderingSet->AddRendering(rendering);
    clientWindow->renderingSet->SetRenderFrame(clientWindow->frame++);
    clientWindow->renderingSet->Commit();

    renderer->Start(clientWindow->renderingSet);

    HANDLED_OK;
  }
  else if (cmd == "gui::mhsample")
  {
    string id  = doc["id"].GetString();

    Filter *filter = getFilter(id);
    if (! filter)
    {
      filter = new MHSampler(Filter::getSource(doc));
      addFilter(id, filter);
      temporaries->Insert(filter->GetName(), filter->getResult());
    }

    MHSampler *sampler = (MHSampler*)filter;
    if (! sampler)
      HANDLED_BUT_ERROR_RETURN("mhsample: filter was not MHSampler");

    KeyedDataObjectP scalarVolume = temporaries->Find(doc["source"].GetString());
    if (! scalarVolume)
      scalarVolume = globals->Find(doc["source"].GetString());

    sampler->Sample(doc, scalarVolume);

    KeyedDataObjectP samples = sampler->getResult();
    
    float m, M;
    samples->get_global_minmax(m, M);

    replyDoc.AddMember("name", rapidjson::Value().SetString(sampler->GetName().data(), sampler->GetName().size()+1), alloc);
    replyDoc.AddMember("key", rapidjson::Value().SetInt(samples->getkey()), alloc);
    replyDoc.AddMember("type", rapidjson::Value().SetInt(2), alloc);
    replyDoc.AddMember("ncomp", rapidjson::Value().SetInt(1), alloc);
    replyDoc.AddMember("min", rapidjson::Value().SetDouble(m), alloc);
    replyDoc.AddMember("max", rapidjson::Value().SetDouble(M), alloc);

    Box *box = samples->get_global_box();
    rapidjson::Value boxv(rapidjson::kArrayType);
    boxv.PushBack(rapidjson::Value().SetDouble(box->get_min()[0]), alloc);
    boxv.PushBack(rapidjson::Value().SetDouble(box->get_max()[0]), alloc);
    boxv.PushBack(rapidjson::Value().SetDouble(box->get_min()[1]), alloc);
    boxv.PushBack(rapidjson::Value().SetDouble(box->get_max()[1]), alloc);
    boxv.PushBack(rapidjson::Value().SetDouble(box->get_min()[2]), alloc);
    boxv.PushBack(rapidjson::Value().SetDouble(box->get_max()[2]), alloc);
    replyDoc.AddMember("box", boxv, alloc);

    replyDoc.AddMember("status",  rapidjson::Value().SetString("ok", 3), alloc);

    HANDLED_OK;
  }
  else if (cmd == "gui::dsample")
  {
    string id  = doc["id"].GetString();

    Filter *filter = getFilter(id);
    if (! filter)
    {
      filter = new DensitySampler(Filter::getSource(doc));
      addFilter(id, filter);
      temporaries->Insert(filter->GetName(), filter->getResult());
    }

    DensitySampler *sampler = (DensitySampler*)filter;
    if (! sampler)
      HANDLED_BUT_ERROR_RETURN("densitysample: filter was not DensitySampler");

    KeyedDataObjectP scalarVolume = temporaries->Find(doc["source"].GetString());
    if (! scalarVolume)
      scalarVolume = globals->Find(doc["source"].GetString());

    sampler->Sample(doc, scalarVolume);

    KeyedDataObjectP samples = sampler->getResult();
    
    float m, M;
    samples->get_global_minmax(m, M);

    replyDoc.AddMember("name", rapidjson::Value().SetString(sampler->GetName().data(), sampler->GetName().size()+1), alloc);
    replyDoc.AddMember("key", rapidjson::Value().SetInt(samples->getkey()), alloc);
    replyDoc.AddMember("type", rapidjson::Value().SetInt(2), alloc);
    replyDoc.AddMember("ncomp", rapidjson::Value().SetInt(1), alloc);
    replyDoc.AddMember("min", rapidjson::Value().SetDouble(m), alloc);
    replyDoc.AddMember("max", rapidjson::Value().SetDouble(M), alloc);

    Box *box = samples->get_global_box();
    rapidjson::Value boxv(rapidjson::kArrayType);
    boxv.PushBack(rapidjson::Value().SetDouble(box->get_min()[0]), alloc);
    boxv.PushBack(rapidjson::Value().SetDouble(box->get_max()[0]), alloc);
    boxv.PushBack(rapidjson::Value().SetDouble(box->get_min()[1]), alloc);
    boxv.PushBack(rapidjson::Value().SetDouble(box->get_max()[1]), alloc);
    boxv.PushBack(rapidjson::Value().SetDouble(box->get_min()[2]), alloc);
    boxv.PushBack(rapidjson::Value().SetDouble(box->get_max()[2]), alloc);
    replyDoc.AddMember("box", boxv, alloc);

    replyDoc.AddMember("status",  rapidjson::Value().SetString("ok", 3), alloc);

    HANDLED_OK;
  }
  else if (cmd == "gui::raysample")
  {
    string id  = doc["id"].GetString();

    Filter *filter = getFilter(id);
    if (! filter)
    {
      filter = new RaycastSampler(Filter::getSource(doc));
      addFilter(id, filter);
      temporaries->Insert(filter->GetName(), filter->getResult());
    }

    RaycastSampler *sampler = (RaycastSampler*)filter;
    if (! sampler)
      HANDLED_BUT_ERROR_RETURN("raysampler: found filter that wasn't RaycastSampler");

    KeyedDataObjectP scalarVolume = temporaries->Find(doc["source"].GetString());
    if (! scalarVolume)
      scalarVolume = globals->Find(doc["source"].GetString());

    sampler->Sample(doc, scalarVolume);

    KeyedDataObjectP samples = sampler->getResult();
    
    float m, M;
    samples->get_global_minmax(m, M);

    replyDoc.AddMember("name", rapidjson::Value().SetString(sampler->GetName().data(), sampler->GetName().size()+1), alloc);
    replyDoc.AddMember("key", rapidjson::Value().SetInt(samples->getkey()), replyDoc.GetAllocator());
    replyDoc.AddMember("type", rapidjson::Value().SetInt(2), replyDoc.GetAllocator());
    replyDoc.AddMember("ncomp", rapidjson::Value().SetInt(1), replyDoc.GetAllocator());
    replyDoc.AddMember("min", rapidjson::Value().SetDouble(m), replyDoc.GetAllocator());
    replyDoc.AddMember("max", rapidjson::Value().SetDouble(M), replyDoc.GetAllocator());

    Box *box = samples->get_global_box();
    rapidjson::Value boxv(rapidjson::kArrayType);
    boxv.PushBack(rapidjson::Value().SetDouble(box->get_min()[0]), replyDoc.GetAllocator());
    boxv.PushBack(rapidjson::Value().SetDouble(box->get_max()[0]), replyDoc.GetAllocator());
    boxv.PushBack(rapidjson::Value().SetDouble(box->get_min()[1]), replyDoc.GetAllocator());
    boxv.PushBack(rapidjson::Value().SetDouble(box->get_max()[1]), replyDoc.GetAllocator());
    boxv.PushBack(rapidjson::Value().SetDouble(box->get_min()[2]), replyDoc.GetAllocator());
    boxv.PushBack(rapidjson::Value().SetDouble(box->get_max()[2]), replyDoc.GetAllocator());
    replyDoc.AddMember("box", boxv, replyDoc.GetAllocator());

    replyDoc.AddMember("status",  rapidjson::Value().SetString("ok", 3), replyDoc.GetAllocator());

    HANDLED_OK;
  }
  else if (cmd == "gui::streamtracer")
  {
    string id  = doc["id"].GetString();

    StreamTracerFilter *streamtracer;

    Filter *filter = getFilter(id);
    if (! filter)
    {

      if (! doc.HasMember("vectorField"))
        HANDLED_BUT_ERROR_RETURN("there's no vectorField name");

      std::string name = doc["vectorField"].GetString();

      VolumeP vectorField = Volume::Cast(globals->Find(name));
      if (! vectorField)
        vectorField = Volume::Cast(temporaries->Find(name));

      if (! vectorField)
        HANDLED_BUT_ERROR_RETURN("unable to find StreamTracer vector dataset");

      filter = new StreamTracerFilter();
      streamtracer = dynamic_cast<StreamTracerFilter*>(filter);

      streamtracer->SetVectorField(vectorField);

      addFilter(id, filter);
      temporaries->Insert(filter->GetName(), filter->getResult());
    }
    else 
      streamtracer = dynamic_cast<StreamTracerFilter*>(filter);

    std::string name = doc["seeds"].GetString();

    ParticlesP seeds = Particles::Cast(temporaries->Find(name));
    if (! seeds)
      seeds = Particles::Cast(temporaries->Find(name));

    if (! seeds)
      HANDLED_BUT_ERROR_RETURN("streamtracer: couldn't find seeds");

    streamtracer->set_max_steps(doc["maxsteps"].GetInt());
    streamtracer->set_stepsize(doc["stepsize"].GetDouble());
    streamtracer->SetMinVelocity(doc["minvelocity"].GetDouble());
    streamtracer->SetMaxIntegrationTime(doc["maxtime"].GetDouble());
    streamtracer->SetMaxIntegrationTime(doc["maxtime"].GetDouble());

    streamtracer->Trace(seeds);

    replyDoc.AddMember("name", rapidjson::Value().SetString(streamtracer->GetName().data(), streamtracer->GetName().size()+1), alloc);
    replyDoc.AddMember("key", rapidjson::Value().SetInt(streamtracer->GetTheStreamlines()->getkey()), replyDoc.GetAllocator());

    HANDLED_OK;
  }
  else if (cmd == "gui::trace2pathlines")
  {
    string id  = doc["id"].GetString();

    Filter *filter = getFilter(id);
    if (! filter)
      HANDLED_BUT_ERROR_RETURN("trace2pathlines - got to trace first");

    StreamTracerFilter *streamtracer = dynamic_cast<StreamTracerFilter*>(filter);

    streamtracer->SetTStart(doc["trimtime"].GetDouble());
    streamtracer->SetDeltaT(doc["trimdeltatime"].GetDouble());

    streamtracer->TraceToPathLines();

    KeyedDataObjectP kdop = streamtracer->getResult();
    
    int type;
    if (kdop->getclass() ==  Volume::ClassType) type = 0;
    else if (kdop->getclass() ==  Triangles::ClassType) type = 1;
    else if (kdop->getclass() ==  Particles::ClassType) type = 2;
    else if (kdop->getclass() ==  PathLines::ClassType) type = 3;
    else type = -1;

    float m, M;
    kdop->get_global_minmax(m, M);

    replyDoc.AddMember("name", rapidjson::Value().SetString(streamtracer->GetName().data(), streamtracer->GetName().size()+1), alloc);
    replyDoc.AddMember("key", rapidjson::Value().SetInt(kdop->getkey()), replyDoc.GetAllocator());
    replyDoc.AddMember("type", rapidjson::Value().SetInt(type), replyDoc.GetAllocator());
    replyDoc.AddMember("ncomp", rapidjson::Value().SetInt(1), replyDoc.GetAllocator());
    replyDoc.AddMember("min", rapidjson::Value().SetDouble(m), replyDoc.GetAllocator());
    replyDoc.AddMember("max", rapidjson::Value().SetDouble(M), replyDoc.GetAllocator());

    Box *box = kdop->get_global_box();
    rapidjson::Value boxv(rapidjson::kArrayType);
    boxv.PushBack(rapidjson::Value().SetDouble(box->get_min()[0]), replyDoc.GetAllocator());
    boxv.PushBack(rapidjson::Value().SetDouble(box->get_max()[0]), replyDoc.GetAllocator());
    boxv.PushBack(rapidjson::Value().SetDouble(box->get_min()[1]), replyDoc.GetAllocator());
    boxv.PushBack(rapidjson::Value().SetDouble(box->get_max()[1]), replyDoc.GetAllocator());
    boxv.PushBack(rapidjson::Value().SetDouble(box->get_min()[2]), replyDoc.GetAllocator());
    boxv.PushBack(rapidjson::Value().SetDouble(box->get_max()[2]), replyDoc.GetAllocator());
    replyDoc.AddMember("box", boxv, replyDoc.GetAllocator());

    HANDLED_OK;
  }
  else if (cmd == "gui::remove_filter")
  {
    string id  = doc["id"].GetString();
    removeFilter(id);
    HANDLED_OK;
  }
  else if (cmd == "gui::remove_window")
  {
    string id  = doc["id"].GetString();
    removeClientWindow(id);
    HANDLED_OK;
  }

  return false;
}

}
