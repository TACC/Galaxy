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

#include <string>
#include <dlfcn.h>
#include <map>

#include "Application.h"
#include "SOManager.h"

using namespace std;

namespace gxy
{

WORK_CLASS_TYPE(SOManager::SOMsg)

SOManager::SOManager()
{
  SOMsg::Register();
}

void
SOManager::Load(string soname)
{
  for (auto i : somap)
    if (i->name == soname)
    {
      i->refknt++;
      return;
    }

  SOMsg l(LOAD, soname);
  l.Broadcast(true, true);
}

void
SOManager::Release(string soname)
{
  std::vector<std::shared_ptr<SO>>::iterator i;

  for (i = somap.begin(); i != somap.end(); i++)
    if ((*i)->name == soname)
      break;

  if (i == somap.end())
  {
    std::cerr << soname << " not loaded\n";
    return;
  }

  (*i)->refknt --;
  if ((*i)->refknt <= 0)
  {
    SOMsg l(RELEASE, soname);
    l.Broadcast(true, true);
  }
}

void *
SOManager::GetSym(string soname, string symbol)
{
  for (auto i : somap)
    if (i->name == soname)
      return dlsym(i->handle, symbol.c_str());

  std::cerr << soname << " is not managed\n";
  return NULL;
}

void
SOManager::_release(string soname)
{
  std::vector<std::shared_ptr<SO>>::iterator i;

  for (i = somap.begin(); i != somap.end(); i++)
    if ((*i)->name == soname)
      break;

  if (i == somap.end())
    cerr << "releasing unknown SO: " << soname << "\n";
  else
    somap.erase(i);
}

void
SOManager::_load(string soname)
{
  void *handle = dlopen(const_cast<char *>(soname.c_str()), RTLD_NOW);
  if (!handle)
  {
    std::cerr << "Error opening SO " << soname << ": " << dlerror() << "\n";
    return;
  }

  void *(*init)();
  init = (void *(*)()) dlsym(handle, const_cast<char *>("init"));
  if (! init)
  {
    std::cerr << "Error accessing init procedure in SO " << soname << ": " << dlerror() << "\n";
    dlclose(handle);
    return;
  }

  init();

  somap.emplace_back(new SO(handle, soname));
}

SOManager::SOMsg::SOMsg(so_task t, string soname) : SOMsg(sizeof(t) + soname.size() + 1)
{
  unsigned char *ptr = (unsigned char *)get();
  *(so_task *)ptr = t;
  ptr += sizeof(so_task);
  memcpy(ptr, soname.c_str(), soname.size() + 1);
}

bool
SOManager::SOMsg::CollectiveAction(MPI_Comm coll_comm, bool isRoot)
{
  unsigned char *ptr = (unsigned char *)get();
  so_task t = *(so_task*)ptr;
  ptr += sizeof(t);
  string soname((char *)ptr);
  if (t == SOManager::LOAD)
    GetTheApplication()->_loadSO(soname);
  else
    GetTheApplication()->_releaseSO(soname);
  return false;
}

}
