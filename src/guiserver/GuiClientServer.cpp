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

using namespace gxy;

namespace gxy
{

extern "C" void
init()
{
  GuiClientServer::init();
  MHSampler::init();
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
  ServerRendering::RegisterClass();
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

bool
GuiClientServer::handle(string line, string& reply)
{
  DatasetsP theDatasets = Datasets::Cast(MultiServer::Get()->GetGlobal("global datasets"));
  if (! theDatasets)
  {
    theDatasets = Datasets::NewP();
    MultiServer::Get()->SetGlobal("global datasets", theDatasets);
  }

  Document doc;
  if (doc.Parse<0>(line.c_str()).HasParseError())
  {
    reply = "error parsing json command";
    return true;
  }

  string cmd = doc["cmd"].GetString();

  if (cmd == "gui::import")
  {
    if (! theDatasets->LoadFromJSON(doc))
    {
      reply = "error loading JSON";
      return true;
    }

    theDatasets->Commit();

    reply = std::string("ok");
    return true;
  }
  else if (cmd == "gui::list")
  {
    if (! theDatasets)
    {
      reply = std::string("unable to access global datasets");
      exit(1);
    }

    reply = "ok ";

    rapidjson::Document doc;
    doc.SetObject();

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

      rapidjson::Value dset(rapidjson::kObjectType);
      dset.AddMember("name", rapidjson::Value().SetString(it->c_str(), it->length()+1), doc.GetAllocator());
      dset.AddMember("key", rapidjson::Value().SetInt(kdop->getkey()), doc.GetAllocator());
      dset.AddMember("type", rapidjson::Value().SetInt(type), doc.GetAllocator());
      dset.AddMember("ncomp", rapidjson::Value().SetInt(ncomp), doc.GetAllocator());
      dset.AddMember("min", rapidjson::Value().SetDouble(m), doc.GetAllocator());
      dset.AddMember("max", rapidjson::Value().SetDouble(M), doc.GetAllocator());

      Box *box = kdop->get_global_box();
      rapidjson::Value boxv(rapidjson::kArrayType);
      boxv.PushBack(rapidjson::Value().SetDouble(box->get_min()[0]), doc.GetAllocator());
      boxv.PushBack(rapidjson::Value().SetDouble(box->get_max()[0]), doc.GetAllocator());
      boxv.PushBack(rapidjson::Value().SetDouble(box->get_min()[1]), doc.GetAllocator());
      boxv.PushBack(rapidjson::Value().SetDouble(box->get_max()[1]), doc.GetAllocator());
      boxv.PushBack(rapidjson::Value().SetDouble(box->get_min()[2]), doc.GetAllocator());
      boxv.PushBack(rapidjson::Value().SetDouble(box->get_max()[2]), doc.GetAllocator());
      dset.AddMember("box", boxv, doc.GetAllocator());

      array.PushBack(dset, doc.GetAllocator());
    }

    doc.AddMember("Datasets", array, doc.GetAllocator());

    rapidjson::StringBuffer strbuf;
    rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
    doc.Accept(writer);

    reply = reply + strbuf.GetString();

    return true;
  }
  else if (cmd == "gui::initWindow")
  {
    string id  = doc["id"].GetString();
    ClientWindow *clientWindow = getClientWindow(id);
    if (clientWindow)
      reply = "window already initialized";
    else
      addClientWindow(id, new ClientWindow);
    return true;
  }
  else if (cmd == "gui::visualization")
  {
    string id  = doc["id"].GetString();
    ClientWindow *clientWindow = getClientWindow(id);
    if (! clientWindow)
    {
      reply = "window has not been initialized\n";
      return true;
    }

    clientWindow->visualization = Visualization::NewP();
    clientWindow->visualization->LoadFromJSON(doc["Visualization"]);
    clientWindow->visualization->Commit();

    cmd = "Got a visualization";
    return true;
  }
  else if (cmd == "gui::camera")
  {
    string id  = doc["id"].GetString();
    ClientWindow *clientWindow = getClientWindow(id);
    if (! clientWindow)
    {
      reply = "window has not been initialized\n";
      return true;
    }

    clientWindow->camera->LoadFromJSON(doc["Camera"]);
    clientWindow->camera->Commit();

    cmd = "Got a camera";
    return true;
  }
  else if (cmd == "gui::render")
  {
    string id  = doc["id"].GetString();
    ClientWindow *clientWindow = getClientWindow(id);
    if (! clientWindow)
    {
      reply = "window has not been initialized\n";
      return true;
    }

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
    return true;
  }
  else if (cmd == "gui::sample")
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
      reply = std::string(cmd + ": found filter that wasn't MHSampler");

    sampler->Sample(doc);

    KeyedDataObjectP kdop = sampler->getResult();
    
    rapidjson::Document doc;
    doc.SetObject();

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

    doc.AddMember("name", rapidjson::Value().SetString("(none)", 7), doc.GetAllocator());
    doc.AddMember("key", rapidjson::Value().SetInt(kdop->getkey()), doc.GetAllocator());
    doc.AddMember("type", rapidjson::Value().SetInt(type), doc.GetAllocator());
    doc.AddMember("ncomp", rapidjson::Value().SetInt(ncomp), doc.GetAllocator());
    doc.AddMember("min", rapidjson::Value().SetDouble(m), doc.GetAllocator());
    doc.AddMember("max", rapidjson::Value().SetDouble(M), doc.GetAllocator());

    Box *box = kdop->get_global_box();
    rapidjson::Value boxv(rapidjson::kArrayType);
    boxv.PushBack(rapidjson::Value().SetDouble(box->get_min()[0]), doc.GetAllocator());
    boxv.PushBack(rapidjson::Value().SetDouble(box->get_max()[0]), doc.GetAllocator());
    boxv.PushBack(rapidjson::Value().SetDouble(box->get_min()[1]), doc.GetAllocator());
    boxv.PushBack(rapidjson::Value().SetDouble(box->get_max()[1]), doc.GetAllocator());
    boxv.PushBack(rapidjson::Value().SetDouble(box->get_min()[2]), doc.GetAllocator());
    boxv.PushBack(rapidjson::Value().SetDouble(box->get_max()[2]), doc.GetAllocator());
    doc.AddMember("box", boxv, doc.GetAllocator());

    doc.AddMember("status",  rapidjson::Value().SetString("ok", 3), doc.GetAllocator());

    rapidjson::StringBuffer strbuf;
    rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
    doc.Accept(writer);

    reply = strbuf.GetString();

    return true;
  }
  else if (cmd == "gui::remove_filter")
  {
    string id  = doc["id"].GetString();
    removeFilter(id);
    reply = "ok";
    return true;
  }
  else if (cmd == "gui::remove_window")
  {
    string id  = doc["id"].GetString();
    removeClientWindow(id);
    reply = "ok";
    return true;
  }

  return false;
}

}
