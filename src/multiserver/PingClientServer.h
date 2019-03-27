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

#pragma once

#include <iostream>
#include <vector>
#include <sstream>

#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

#include "Application.h"
#include "MultiServer.h"
#include "MultiServerHandler.h"

namespace gxy
{

class PingMsg : public Work
{
public:
  PingMsg(std::string m) : PingMsg(m.size() + 1) { strcpy((char *)get(), m.c_str()); }
  ~PingMsg() {}

  WORK_CLASS(PingMsg, true)

public:
  bool Action(int s);
};

class PingClientServer : public MultiServerHandler
{
public:
  PingClientServer(DynamicLibraryP dlp, int cfd, int dfd) : MultiServerHandler(dlp, cfd, dfd) {}
  static void init();
  std::string handle(std::string line);
};


}
