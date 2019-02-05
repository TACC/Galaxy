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

#include <string.h>
#include <pthread.h>

#include "DataClientServer.h"

using namespace gxy;

namespace gxy
{

extern "C" void
init()
{
  DataClientServer::init();
}

extern "C" MultiServerHandler *
new_handler(DynamicLibraryP dlp, int cfd, int dfd)
{
  return new DataClientServer(dlp, cfd, dfd);
}


void
DataClientServer::init()
{
}

std::string
DataClientServer::handle(std::string line)
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
      return std::string("error parsing import command");

    if (! theDatasets->LoadFromJSON(doc))
      return std::string("error loading datasets");
    
    return std::string("ok");
  }
  else if (cmd == "list")
  {
    if (! theDatasets)
      return std::string("unable to access global datasets");

    vector<string> names = theDatasets->GetDatasetNames();
    std::string reply = "ok datasets:\n";
    for (vector<string>::iterator it = names.begin(); it != names.end(); ++it)
    {
      reply.append(*it);
      reply.append("\n");
    }

    return reply;
  }
  else if (cmd == "drop" || cmd == "delete")
  {
    string objName;
    ss >> objName;
    KeyedDataObjectP kop = theDatasets->Find(objName);
    if (kop)
    {
      theDatasets->DropDataset(objName);
      return std::string("ok");
    }
    else
      return std::string("error no such dataset: " + objName);
  }
  else return MultiServerHandler::handle(line);
}

}
