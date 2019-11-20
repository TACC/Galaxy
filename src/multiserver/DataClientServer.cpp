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

#include <string.h>
#include <pthread.h>

#include "DataClientServer.h"
#include "SocketHandler.h"
#include "Volume.h"
#include "PathLines.h"
#include "Triangles.h"
#include "Particles.h"

#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

using namespace gxy;
using namespace rapidjson; 
using namespace std;

namespace gxy
{

extern "C" void
init()
{
  DataClientServer::init();
}

extern "C" DataClientServer *
new_handler(SocketHandler *sh)
{
  return new DataClientServer(sh);
}

void
DataClientServer::init()
{
}

bool
DataClientServer::handle(std::string line, std::string& reply)
{
  DatasetsP theDatasets = Datasets::Cast(MultiServer::Get()->GetGlobal("global datasets"));
  if (! theDatasets)
  {
    theDatasets = Datasets::NewP();
    MultiServer::Get()->SetGlobal("global datasets", theDatasets);
  }

  std::stringstream ss(line);
  std::string cmd;
  ss >> cmd;

  if (cmd == "import")
  {
    std::string remaining;
    std::getline(ss, remaining);
    Document doc;

    if (doc.Parse<0>(remaining.c_str()).HasParseError())
    {
      reply = "error parsing import command";
      return true;
    }

    if (! theDatasets->LoadFromJSON(doc))
    {
      reply = "error loading JSON";
      return true;
    }

    reply = std::string("ok");
    return true;
  }
  else if (cmd == "range")
  {
    string objName;
    ss >> objName;
    KeyedDataObjectP kop = theDatasets->Find(objName);
    if (kop)
    {
      float gmin, gmax;
      kop->get_global_minmax(gmin, gmax);

      std::stringstream rtn;
      rtn << "ok dataset " << objName << " range: " << gmin << " to " << gmax;
      reply = rtn.str();

      return true;
    }
    else
    {
      reply = std::string("error dataset") + objName + " not found";
      return true;
    }
  }
  else if (cmd == "commit")
  {
    string objName;
    ss >> objName;
    KeyedDataObjectP kop = theDatasets->Find(objName);
    if (kop)
    {
      kop->Commit();
      reply = std::string("ok ") + objName + " committed";
      return true;
    }
    else
      reply = std::string("error dataset") + objName + " not found";

    return true;
  }
  else if (cmd == "list")
  {
    if (! theDatasets)
    {
      reply = std::string("unable to access global datasets");
      exit(1);
    }

    reply = "ok datasets:\n";

    vector<string> names = theDatasets->GetDatasetNames();
    for (vector<string>::iterator it = names.begin(); it != names.end(); ++it)
    {
      reply.append(*it);
      reply.append("\n");
    }

    return true;
  }
  else if (cmd == "listWithInfo")
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

    std::cerr << "returning XX " << reply << "\n";

    return true;
  }
  else if (cmd == "drop" || cmd == "delete")
  {
    string objName;
    ss >> objName;
    KeyedDataObjectP kop = theDatasets->Find(objName);
    if (kop)
    {
      theDatasets->DropDataset(objName);
      reply = std::string("ok");
      return true;
    }
    else
    {
      reply = std::string("error no such dataset: " + objName);
      return true;
    }
  }
  else 
    return false;
}

}
