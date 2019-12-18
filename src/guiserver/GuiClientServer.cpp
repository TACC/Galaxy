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

#include <iostream>
#include <vector>
#include <sstream>

using namespace std;

#include <string.h>

#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"

using namespace rapidjson;

#include "GuiClientServer.h"
#include "MHSampler.hpp"
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
  ServerRendering::RegisterClass();
  StreamTracer::Register();
}

GuiClientServer::~GuiClientServer()
{
}

bool
GuiClientServer::Sample(Document& params, std::string& reply)
{
  reply = "OK";
  return true;
}

static std::string 
DocumentToString(rapidjson::Document& doc)
{
  rapidjson::StringBuffer strbuf;
  rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
  doc.Accept(writer);
  return strbuf.GetString();
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
  std::cerr << "REPLY: " << reply << "\n"; \
  return true;                                                                                  \
}

bool
GuiClientServer::handle(string line, string& reply)
{
  Document replyDoc;
  replyDoc.SetObject();

  Document::AllocatorType& alloc = replyDoc.GetAllocator();

  DatasetsP theDatasets = Datasets::Cast(MultiServer::Get()->GetGlobal("global datasets"));
  if (! theDatasets)
  {
    theDatasets = Datasets::NewP();
    MultiServer::Get()->SetGlobal("global datasets", theDatasets);
  }

  Document doc;
  if (doc.Parse<0>(line.c_str()).HasParseError())
    HANDLED_BUT_ERROR_RETURN("error parsing json command");

  string cmd = doc["cmd"].GetString();

  if (cmd == "gui::import")
  {
    if (! theDatasets->LoadFromJSON(doc))
      HANDLED_BUT_ERROR_RETURN("import datasets: error in LoadFromJson");

    theDatasets->Commit();

    HANDLED_OK;
  }
  else if (cmd == "gui::list")
  {
    if (! theDatasets)
      HANDLED_BUT_ERROR_RETURN("list: unable to access global data");

    Value array(kArrayType);

    vector<string> names = theDatasets->GetDatasetNames();
    for (vector<string>::iterator it = names.begin(); it != names.end(); ++it)
    {
      KeyedDataObjectP kdop = theDatasets->Find(std::string(*it));
     
      int type;
      if (kdop->getclass() ==  gxy::Volume::ClassType) type = 0;
      else if (kdop->getclass() ==  gxy::Triangles::ClassType) type = 1;
      else if (kdop->getclass() ==  gxy::Particles::ClassType) type = 2;
      else if (kdop->getclass() ==  gxy::PathLines::ClassType) type = 3;
      else type = -1;

      int ncomp;
      if (type == 0)
      {
        gxy::VolumeP v = gxy::Volume::Cast(kdop);
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

    HANDLED_OK;
  }
  else if (cmd == "gui::initWindow")
  {
    string id  = doc["id"].GetString();
    ClientWindow *clientWindow = getClientWindow(id);
    if (clientWindow)
      HANDLED_BUT_ERROR_RETURN("initWindow: window already initialized")

    addClientWindow(id, new ClientWindow);

    HANDLED_OK;
  }
  else if (cmd == "gui::visualization")
  {
    string id  = doc["id"].GetString();
    ClientWindow *clientWindow = getClientWindow(id);
    if (! clientWindow)
      HANDLED_BUT_ERROR_RETURN("initWindow: window has not been initialized");

    clientWindow->visualization = Visualization::NewP();

    if (! clientWindow->visualization->LoadFromJSON(doc["Visualization"]))
      HANDLED_BUT_ERROR_RETURN("visualization: error in LoadFromJson");

    clientWindow->visualization->Commit();

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

    clientWindow->rendering->SetTheSize(clientWindow->camera->get_width(), clientWindow->camera->get_height());
    clientWindow->rendering->SetHandler(this);

    clientWindow->rendering->SetTheVisualization(clientWindow->visualization);
    clientWindow->rendering->SetTheCamera(clientWindow->camera);
    clientWindow->rendering->SetTheDatasets(theDatasets);
    clientWindow->rendering->Commit();

    RenderingSetP rs = RenderingSet::NewP();
    rs->AddRendering(clientWindow->rendering);
    rs->Commit();
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
    }

    MHSampler *sampler = (MHSampler*)filter;
    if (! sampler)
      HANDLED_BUT_ERROR_RETURN("mhsample: filter was not MHSampler");

    sampler->Sample(doc);

    KeyedDataObjectP kdop = sampler->getResult();
    
    int type;
    if (kdop->getclass() ==  gxy::Volume::ClassType) type = 0;
    else if (kdop->getclass() ==  gxy::Triangles::ClassType) type = 1;
    else if (kdop->getclass() ==  gxy::Particles::ClassType) type = 2;
    else if (kdop->getclass() ==  gxy::PathLines::ClassType) type = 3;
    else type = -1;

    int ncomp;
    if (type == 0)
    {
      gxy::VolumeP v = gxy::Volume::Cast(kdop);
      ncomp = v->get_number_of_components();
    }
    else
      ncomp = -1;

    float m, M;
    kdop->get_global_minmax(m, M);

    replyDoc.AddMember("name", rapidjson::Value().SetString("(none)", 7), alloc);
    replyDoc.AddMember("key", rapidjson::Value().SetInt(kdop->getkey()), alloc);
    replyDoc.AddMember("type", rapidjson::Value().SetInt(type), alloc);
    replyDoc.AddMember("ncomp", rapidjson::Value().SetInt(ncomp), alloc);
    replyDoc.AddMember("min", rapidjson::Value().SetDouble(m), alloc);
    replyDoc.AddMember("max", rapidjson::Value().SetDouble(M), alloc);

    Box *box = kdop->get_global_box();
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
    }

    RaycastSampler *sampler = (RaycastSampler*)filter;
    if (! sampler)
      HANDLED_BUT_ERROR_RETURN("raysampler: found filter that wasn't RaycastSampler");

    sampler->Sample(doc);

    KeyedDataObjectP kdop = sampler->getResult();
    
    int type;
    if (kdop->getclass() ==  gxy::Volume::ClassType) type = 0;
    else if (kdop->getclass() ==  gxy::Triangles::ClassType) type = 1;
    else if (kdop->getclass() ==  gxy::Particles::ClassType) type = 2;
    else if (kdop->getclass() ==  gxy::PathLines::ClassType) type = 3;
    else type = -1;

    float m, M;
    kdop->get_global_minmax(m, M);

    replyDoc.AddMember("name", rapidjson::Value().SetString("(none)", 7), replyDoc.GetAllocator());
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
      if (! doc.HasMember("vectorFieldKey"))
        HANDLED_BUT_ERROR_RETURN("there's no vectorField");

      Key key = doc["vectorFieldKey"].GetInt();
      VolumeP vectorField = Volume::Cast(KeyedDataObject::GetByKey(key));

      filter = new StreamTracerFilter();
      streamtracer = dynamic_cast<StreamTracerFilter*>(filter);

      streamtracer->SetVectorField(vectorField);

      addFilter(id, filter);
    }
    else
      streamtracer = dynamic_cast<StreamTracerFilter*>(filter);

    Key key = doc["seedsKey"].GetInt();
    ParticlesP seeds = Particles::Cast(KeyedDataObject::GetByKey(key));
    if (! seeds)
      HANDLED_BUT_ERROR_RETURN("streamtracer: couldn't find seeds");

    streamtracer->set_max_steps(doc["maxsteps"].GetInt());
    streamtracer->set_stepsize(doc["stepsize"].GetDouble());
    streamtracer->SetMinVelocity(doc["minvelocity"].GetDouble());
    streamtracer->SetMaxIntegrationTime(doc["maxtime"].GetDouble());
    streamtracer->SetMaxIntegrationTime(doc["maxtime"].GetDouble());

    streamtracer->Trace(seeds);

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
    if (kdop->getclass() ==  gxy::Volume::ClassType) type = 0;
    else if (kdop->getclass() ==  gxy::Triangles::ClassType) type = 1;
    else if (kdop->getclass() ==  gxy::Particles::ClassType) type = 2;
    else if (kdop->getclass() ==  gxy::PathLines::ClassType) type = 3;
    else type = -1;

    float m, M;
    kdop->get_global_minmax(m, M);

    replyDoc.AddMember("name", rapidjson::Value().SetString("(none)", 7), replyDoc.GetAllocator());
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
