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

namespace gxy
{

thread_local MultiServerHandler* thread_msh;

MultiServerHandler*
MultiServerHandler::GetTheThreadMultiServerHandler() { return gxy::thread_msh; }

bool
MultiServerHandler::RunServer()
{
  thread_msh = this;
  
  char *libname; int sz;
  CRecv(libname, sz);

  std::string ok("ok");
  CSend(ok.c_str(), ok.length()+1);

  dlp = MultiServer::Get()->getTheDynamicLibraryManager()->Load(libname);
  free(libname);

  if (!dlp)
    return false;

  server_func server = (server_func)dlp->GetSym("server");
  if (! server)
    return false;

  if (! server(this))
    return false;

  return true;
}

static void
brk(){ std::cerr << "break\n"; }

bool
MultiServerHandler::handle(char *s)
{
  std::stringstream ss(s);
  std::string cmd;

  ss >> cmd;
  if (cmd == "break") { brk(); return true; }
  else { std::cerr << "huh?"; return false; }
}

}
