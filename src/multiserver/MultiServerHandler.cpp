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
#include <string>
#include <sstream>

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <math.h>
#include <string.h>
#include <fcntl.h>

#include "MultiServer.h"
#include "MultiServerHandler.h"
#include "Datasets.h"

namespace gxy
{

thread_local MultiServerHandler* thread_msh;

MultiServerHandler*
MultiServerHandler::GetTheThreadMultiServerHandler() { return gxy::thread_msh; }

static void brk(){ std::cerr << "break\n"; } // for debugging

std::string
MultiServerHandler::handle(std::string line)
{
  if (line == "break")
  {
    brk();
    return std::string("ok");
  }
  else
  {
    return std::string("error unrecognized command: ") + line;
  }
}

bool
MultiServerHandler::RunServer()
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
    std::string line, reply;

    if (! CRecv(line))
      break;

    cerr << "received " << line << "\n";

    if (line == "quit")
    {
      client_done = true;
      reply = "ok";
    }
    if (line == "clear")
    {
      MultiServer::Get()->ClearGlobals();
      reply = "ok";
    }
    else
      reply = handle(line);

    CSend(reply.c_str(), reply.length()+1);
  }

  return true;
}

}
