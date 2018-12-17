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

#pragma once

#include <string>
#include <dlfcn.h>
#include <vector>
#include <memory>

namespace gxy
{

class SOManager
{
  enum so_task { LOAD, RELEASE };

  class SOMsg : public Work
  {
  public:
    SOMsg(so_task t, std::string s);
    WORK_CLASS(SOMsg, true)

  public:
    bool CollectiveAction(MPI_Comm coll_comm, bool isRoot);
  };
  
  struct SO
  {
    SO(void *h, std::string n) : name(n), handle(h), refknt(1) {}
    ~SO() { std::cerr << "closing " << name << "\n"; dlclose(handle); }

    void *handle;
    std::string name;
    int refknt;
  };

public:
  SOManager();

  void Load(std::string);
  void Release(std::string);
  void *GetSym(std::string, std::string);

  void _load(std::string s);
  void _release(std::string s);

  std::vector<std::shared_ptr<SO>> somap;
};

}
