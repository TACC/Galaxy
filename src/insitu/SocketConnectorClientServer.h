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

#pragma once

#include <iostream>
#include <vector>
#include <map>
#include <sstream>

#include <string.h>
#include <pthread.h>

#include "Application.h"
#include "MultiServerHandler.h"
#include "Datasets.h"
#include "SocketConnector.hpp"

#include "rapidjson/document.h"

namespace gxy
{

class SocketConnectorClientServer : public MultiServerHandler
{
public:
  SocketConnectorClientServer(SocketHandler *sh) : MultiServerHandler(sh) {}
  ~SocketConnectorClientServer() { if (connector) { connector->Wait(); connector = NULL; } }

  static void init();
  bool handle(std::string line, std::string& reply) override;

  virtual void Notify(GalaxyObject* o, ObserverEvent id, void *cargo) override;

private:

  SocketConnectorP connector = NULL;
  int timeout = 60;

  pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
  pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

  void InitializeVolumes(rapidjson::Document&);

  std::map<std::string, VolumeP> variables;

  class InitializeVolumesMsg : public Work
  {
  public:
    enum todo {Open, Close, Accept};

    InitializeVolumesMsg(std::string s) : InitializeVolumesMsg(s.size() + 1)
    {
      strcpy((char *)contents->get(), s.c_str());
    }

    WORK_CLASS(InitializeVolumesMsg, true);

    bool CollectiveAction(MPI_Comm coll_comm, bool isRoot);
  };

};



}

