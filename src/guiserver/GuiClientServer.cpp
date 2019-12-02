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

using namespace gxy;

namespace gxy
{

extern "C" void
init()
{
  GuiClientServer::init();
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

#if 0
  string id  = doc["id"].GetString();

  GuiClient *client = clients[id];
  if (! client)
  {
    client = new GuiClient;
    clients[id] = client;
  }
#endif

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
    GuiClient *client = clients[id];
    if (client)
    {
      reply = "window already initialized";
      return true;
    }
    else
    {
      if (! client)
      {
        client = new GuiClient;
        clients[id] = client;
      }
    }
  }
  else if (cmd == "gui::visualization")
  {
    string id  = doc["id"].GetString();
    GuiClient *client = clients[id];
    if (! client)
    {
      reply = "window has not been initialized\n";
      return true;
    }

    client->visualization = Visualization::NewP();
    client->visualization->LoadFromJSON(doc["Visualization"]);
    client->visualization->Commit();

    cmd = "Got a visualization";
    return true;
  }
  else if (cmd == "gui::camera")
  {
    string id  = doc["id"].GetString();
    GuiClient *client = clients[id];
    if (! client)
    {
      reply = "window has not been initialized\n";
      return true;
    }

    // client->camera = Camera::NewP();
    client->camera->LoadFromJSON(doc["Camera"]);
    client->camera->Commit();

    cmd = "Got a camera";
    return true;
  }
  else if (cmd == "gui::render")
  {
    string id  = doc["id"].GetString();
    GuiClient *client = clients[id];
    if (! client)
    {
      reply = "window has not been initialized\n";
      return true;
    }

    client->rendering->SetTheSize(client->camera->get_width(), client->camera->get_height());
    client->rendering->SetHandler(this);

    client->rendering->SetTheVisualization(client->visualization);
    client->rendering->SetTheCamera(client->camera);
    client->rendering->SetTheDatasets(theDatasets);
    client->rendering->Commit();

    RenderingSetP rs = RenderingSet::NewP();
    rs->AddRendering(client->rendering);
    rs->Commit();
    renderer->Start(client->renderingSet);
  }
  else if (cmd == "gui::sample")
  {
    return Sample(doc, reply);
  }
  else
    return false;
}

}
