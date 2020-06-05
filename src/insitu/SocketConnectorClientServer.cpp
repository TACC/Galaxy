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
#include <regex>

using namespace std;

#include <string.h>
#include <pthread.h>

#include "SocketConnector.hpp"
#include "SocketConnectorClientServer.h"

#include "Volume.h"

#include "rapidjson/document.h"

void dbg_stop() {}

namespace gxy
{

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
      connector->addVariable(name, volume);
    }

    connector->set_wait(timeout * 1000);
    connector->Commit();

    connector->Open();

    reply = "ok";
    return true;
  }
  else if (cmd == "accept;")
  {
    if (connector)
    {
      connector->Accept();
      reply = "ok";
      connector->publish(theDatasets);
    }
    else
      reply = "connector not created";

    return true;
  }
  else if (cmd == "commit;")
  {
    if (connector)
    {
      connector->commit_datasets();
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

}
