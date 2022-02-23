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

#include <iostream>
#include <vector>
#include <sstream>
#include <regex>

using namespace std;

#include <string.h>
#include <pthread.h>

#include "SocketConnector.hpp"
#include "SocketConnectorClientServer.h"

#include "Volume.h"
#include "Datasets.h"

#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"


void dbg_stop() {}

namespace gxy
{

WORK_CLASS_TYPE(SocketConnectorClientServer::InitializeVolumesMsg);

extern "C" void
init()
{
  SocketConnectorClientServer::init();
}

extern "C" MultiServerHandler *
new_handler(SocketHandler *sh)
{
  return new SocketConnectorClientServer(sh);
}

void
SocketConnectorClientServer::init()
{ 
  SocketConnector::Register();
  InitializeVolumesMsg::Register();
}

bool
SocketConnectorClientServer::handle(string line, string& reply)
{
  // std::cerr << "InSitu Handle: " << line << "\n";

  DatasetsP theDatasets = Datasets::Cast(MultiServer::Get()->GetGlobal("global datasets"));
  if (! theDatasets)
  {
    theDatasets = Datasets::NewP();
    MultiServer::Get()->SetGlobal("global datasets", theDatasets);
  }

  stringstream ss(line);
  string cmd;
  ss >> cmd;

  if (cmd == "new") 
  {
    string datadesc;
    std::getline(ss, datadesc, (char)EOF);

    datadesc = regex_replace(datadesc, std::regex(";[ \t]*$"), "");

    rapidjson::Document json;
    json.Parse(datadesc.c_str());

    connector = SocketConnector::NewP();
    connector->set_port(1900);                      // Each participant will bump this by their rank!

    // For every incoming variable, create as fresh
    // Volume object.   Any operations that refer to
    // a prior value will be OK (I hope)

    for (auto& v: json["variables"].GetArray())
    {
      std::string name = v["name"].GetString();
      VolumeP volume = Volume::NewP();
      v.AddMember("key", rapidjson::Value().SetInt(volume->getkey()), json.GetAllocator());
      variables[name] = volume;
    }

    // Initialize volumes

    InitializeVolumes(json);

    // Publish them

    for (auto a = variables.begin(); a != variables.end(); a++)
      if (! theDatasets->Find(a->first))
        theDatasets->Insert(a->first, a->second);

    connector->set_wait(timeout * 1000);
    connector->Commit();

    connector->Open();

    reply = "ok";
    return true;
  }
  else if (cmd == "accept")
  {
    std::string arg;
    ss >> arg;

    arg = regex_replace(arg, std::regex(";"), "");

    if (connector)
    {
      connector->Accept(variables[arg]);
      variables[arg]->Commit();
      reply = "ok";
    }
    else
      reply = "connector not created";

    return true;
  } else if (cmd == "commit;") {
    if (connector)
    {
      //connector->commit_datasets();
      reply = "ok";
    }
    else
      reply = "connector not created";

    return true;
  }
  else if (cmd == "close;")
  {
    if (connector)
    {
      connector->Close();
      reply = "ok";
    }
    else
      reply = "connector not created";

    return true;
  }
  else if (cmd == "timeout")
  {
    ss >> timeout;
    return true;
  }
  else
    return false;
}

void
SocketConnectorClientServer::Notify(GalaxyObject* o, ObserverEvent id, void *cargo)
{
  switch(id)
  {
    case ObserverEvent::Updated:
    {
      std::cerr << "SocketConnectorClientServer has received an Updated notice\n";
      return;
    }

    default:
      super::Notify(o, id, cargo);
      return;
  }
}

void
SocketConnectorClientServer::InitializeVolumes(rapidjson::Document& desc)
{ 
  rapidjson::StringBuffer strbuf;
  rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
  desc.Accept(writer);
  std::string line = strbuf.GetString();
  
  InitializeVolumesMsg msg(line);
  msg.Broadcast(true, true);
}

bool
SocketConnectorClientServer::InitializeVolumesMsg::CollectiveAction(MPI_Comm comm, bool is_root)
{
  char *p = (char *)contents->get();

  rapidjson::Document desc;
  desc.Parse(p);

  rapidjson::Value& me = desc["partitions"][GetTheApplication()->GetRank()];

  for (int i = 0; i < desc["variables"].Size(); i++)
  {
    rapidjson::Value& v = desc["variables"][i];
    Key key = (Key)v["key"].GetInt();
    std::string name = std::string(v["name"].GetString());

    VolumeP volume = Volume::GetByKey(key);
    
    volume->set_number_of_components(v["vector"].GetBool() ? 3 : 1);
    volume->set_type(v["float"].GetBool() ? Volume::FLOAT : Volume::UCHAR);

    float gorigin[] = {float(desc["origin"][0].GetDouble()), 
                       float(desc["origin"][1].GetDouble()), 
                       float(desc["origin"][2].GetDouble())};
    
    float spacing[] = {float(desc["spacing"][0].GetDouble()), 
                       float(desc["spacing"][1].GetDouble()), 
                       float(desc["spacing"][2].GetDouble())};
    
    int partition_grid[] = {int(desc["partition grid"][0].GetInt()), 
                            int(desc["partition grid"][1].GetInt()), 
                            int(desc["partition grid"][2].GetInt())};
    
    int global_extent[] = {int(desc["extent"][0].GetInt()), 
                           int(desc["extent"][1].GetInt()), 
                           int(desc["extent"][2].GetInt()), 
                           int(desc["extent"][3].GetInt()), 
                           int(desc["extent"][4].GetInt()), 
                           int(desc["extent"][5].GetInt())};
    
    int global_counts[] = {(global_extent[1] - global_extent[0]) + 1,
                           (global_extent[3] - global_extent[2]) + 1,
                           (global_extent[5] - global_extent[4]) + 1};

    int partition_extent[] = {int(me["extent"][0].GetInt()),
                              int(me["extent"][1].GetInt()),
                              int(me["extent"][2].GetInt()),
                              int(me["extent"][3].GetInt()),
                              int(me["extent"][4].GetInt()),
                              int(me["extent"][5].GetInt())};

    int partition_neighbors[] = {int(me["neighbors"][0].GetInt()),
                                 int(me["neighbors"][1].GetInt()),
                                 int(me["neighbors"][2].GetInt()),
                                 int(me["neighbors"][3].GetInt()),
                                 int(me["neighbors"][4].GetInt()),
                                 int(me["neighbors"][5].GetInt())};

    int partition_ghosts[] = {int(me["ghosts"][0].GetInt()),
                              int(me["ghosts"][1].GetInt()),
                              int(me["ghosts"][2].GetInt()),
                              int(me["ghosts"][3].GetInt()),
                              int(me["ghosts"][4].GetInt()),
                              int(me["ghosts"][5].GetInt())};

    int gcounts[] = {(global_extent[1] - global_extent[0]) + 1,
                     (global_extent[3] - global_extent[2]) + 1,
                     (global_extent[5] - global_extent[4]) + 1};

    int lcounts[] = {((partition_extent[1] - partition_extent[0]) + 1) - (partition_ghosts[0] + partition_ghosts[1]),
                     ((partition_extent[3] - partition_extent[2]) + 1) - (partition_ghosts[2] + partition_ghosts[3]),
                     ((partition_extent[5] - partition_extent[4]) + 1) - (partition_ghosts[4] + partition_ghosts[5])};

    int loffset[] = {partition_extent[0] + partition_ghosts[0],
                     partition_extent[2] + partition_ghosts[2],
                     partition_extent[4] + partition_ghosts[4]};

    volume->set_global_partitions(partition_grid[0], partition_grid[1], partition_grid[2]);
    volume->set_global_counts(gcounts[0], gcounts[1], gcounts[2]);
    volume->set_global_origin(gorigin[0], gorigin[1], gorigin[2]);
    volume->set_deltas(spacing[0], spacing[1], spacing[2]);
    volume->set_local_offset(partition_extent[0] + partition_ghosts[0], 
                             partition_extent[2] + partition_ghosts[2], 
                             partition_extent[4] + partition_ghosts[4]);
    volume->set_local_counts(lcounts[0], lcounts[1], lcounts[2]);
    volume->set_neighbors(partition_neighbors);

    float go[] = {gorigin[0] + spacing[0], gorigin[1] + spacing[1], gorigin[2] + spacing[2]};
    int   gc[] = {gcounts[0] - 2, gcounts[1] - 2, gcounts[2] - 2};
        
    float lo[] =
    {     
      gorigin[0] + (loffset[0] * spacing[0]),
      gorigin[1] + (loffset[1] * spacing[1]),
      gorigin[2] + (loffset[2] * spacing[2])
    };

    Box lbox(lo, (int *)&lcounts, (float *)&spacing);
    Box gbox(go, (int *)&gc, (float *)&spacing);

    volume->set_boxes(lbox, gbox);

    volume->Allocate();
  }

  return false;
}

}
