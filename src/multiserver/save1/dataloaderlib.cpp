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

#include "Datasets.h"
#include "MultiServer.h"
#include "MultiServerHandler.h"

using namespace gxy;

#include "rapidjson/filereadstream.h"
using namespace rapidjson;

extern "C" void
init()
{
  extern void InitializeData();
  InitializeData();
}

extern "C" bool
server(MultiServerHandler *handler)
{
  DatasetsP theDatasets = Datasets::Cast(MultiServer::Get()->GetGlobal("global datasets"));
  if (! theDatasets)
  {
    theDatasets = Datasets::NewP();
    MultiServer::Get()->SetGlobal("global datasets", theDatasets);
  }

  bool client_done = false;
  while (! client_done)
  {
    char *buf; int sz;
    if (! handler->CRecv(buf, sz))
      break;

    cerr << "received " << buf << "\n";

    std::string reply("ok");

    if (!strncmp(buf, "import", strlen("import")))
    {
      Document doc;
      doc.Parse(buf+strlen("import"));
      theDatasets->LoadFromJSON(doc);
    }
    else if (!strncmp(buf, "list", strlen("list")))
    {
      if (! theDatasets)
      {
        reply = "unable to access datasets";
      }
      else
      {
        vector<string> names = theDatasets->GetDatasetNames();

        reply = "datasets:\n";
        for (vector<string>::iterator it = names.begin(); it != names.end(); ++it)
        {
          reply.append(*it);
          reply.append("\n");
        }
      }
    }
    else if (!strncmp(buf, "drop", strlen("drop")))
    {
      char objName[256], cmd[256];
      sscanf(buf, "%s%s", cmd, objName);
      theDatasets->DropDataset(objName);
    }
    else if (!strncmp(buf, "delete", strlen("delete")))
    {
      char objName[256], cmd[256];
      sscanf(buf, "%s%s", cmd, objName);
      KeyedDataObjectP kop = theDatasets->Find(objName);
      if (kop)
      {
        theDatasets->DropDataset(objName);
        Delete(kop);
      }
      else
        cerr << "couldn't find it in theDatasets\n";
    }
    else if (!strcmp(buf, "quit"))
    {
      client_done = true;
    }
    else
      cerr << "huh?\n";

    handler->CSend(reply.c_str(), reply.length()+1);
    free(buf);
  }

  return true;
}
