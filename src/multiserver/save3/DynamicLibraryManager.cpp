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
// ========================================================================== //

#include <string>
#include <vector>
#include <string.h>
#include <dlfcn.h>
#include <map>

#include "Application.h"
#include "MultiServer.h"
#include "DynamicLibraryManager.h"

using namespace std;

namespace gxy
{

KEYED_OBJECT_CLASS_TYPE(DynamicLibrary)
WORK_CLASS_TYPE(DynamicLibrary::CommitMsg)
WORK_CLASS_TYPE(DynamicLibraryManager::FlushMsg)

DynamicLibrary::~DynamicLibrary()
{
  std::cerr << GetTheApplication()->GetRank() << ": closing " << name << "\n";
  dlclose(handle);
}

void
DynamicLibrary::Register()
{
  CommitMsg::Register();
};

int
DynamicLibrary::serialSize()
{
  return super::serialSize() + name.size() + 1;
}

unsigned char *
DynamicLibrary::serialize(unsigned char *p)
{
  p = super::serialize(p);
  strcpy((char *)p, name.c_str());
  return p + name.size() + 1;
}

unsigned char *
DynamicLibrary::deserialize(unsigned char *p)
{
  p = super::deserialize(p);
  name = (char *)p;
  return p + name.size() + 1;
}

bool
DynamicLibrary::local_commit(MPI_Comm c)
{
  handle = dlopen(const_cast<char *>(name.c_str()), RTLD_NOW);
  if (!handle)
  {
    std::cerr << "Error opening SO " << name << ": " << dlerror() << "\n";
    return true;
  }

  void *(*init)();
  init = (void *(*)()) dlsym(handle, const_cast<char *>("init"));
  if (! init)
  {
    std::cerr << "Error accessing init procedure in SO " << name << ": " << dlerror() << "\n";
    dlclose(handle);
    return true;
  }

  init();

  return false;
}

void
DynamicLibrary::Commit()
{
  CommitMsg msg(this);
  msg.Broadcast(true, true);
}

DynamicLibrary::CommitMsg::CommitMsg(DynamicLibrary* kdl) : DynamicLibrary::CommitMsg::CommitMsg(kdl->SerialSize())
{
  unsigned char *p = (unsigned char *)get();
  kdl->Serialize(p);
}

bool
DynamicLibrary::CommitMsg::CollectiveAction(MPI_Comm c, bool isRoot)
{
  unsigned char *p = (unsigned char *)get();
  Key k = *(Key *)p;
  p += sizeof(Key);

  KeyedObjectP kdl = GetTheKeyedObjectFactory()->get(k);

  if (! isRoot)
    kdl->deserialize(p);

  if (kdl->local_commit(c)) return true;

  MultiServer::Get()->getTheDynamicLibraryManager()->post(DynamicLibrary::Cast(kdl));

  return false;
}

void
DynamicLibraryManager::Flush()
{
  // Note - DLs are KeyedObjects, so are reffed by the KeyedObject map
  // by the DynamicLibraryManager loadmap, and by any objects that require 
  // them.  The iterator is a third. So they can be unloaded if their
  // refcount is 3.

  vector<Key> unloadables;

  for (auto i : loadmap)
    if (i.use_count() == 3)
      unloadables.push_back(i->getkey());

  
  if (unloadables.size() > 0)
  { 
    FlushMsg msg(unloadables);
    msg.Broadcast(true, true);
  }
}

DynamicLibraryManager::FlushMsg::FlushMsg(vector<Key> unloadables) :
  DynamicLibraryManager::FlushMsg::FlushMsg(sizeof(int) + unloadables.size() * sizeof(Key))
{
  unsigned char *p = (unsigned char *)get();
  *(int *)p = unloadables.size();
  p += sizeof(int);
  memcpy(p, unloadables.data(), unloadables.size()*sizeof(Key));
}
  
bool
DynamicLibraryManager::FlushMsg::CollectiveAction(MPI_Comm c, bool isRoot)
{
  unsigned char *p = (unsigned char *)get();
  int n = *(int *)p;
  p += sizeof(int);

  MultiServer::Get()->getTheDynamicLibraryManager()->flush(n, (Key *)p);
  return false;
}

void
DynamicLibraryManager::flush(int n, Key *keys)
{
  for (auto i = 0; i < n; i++)
  {
    Key key = keys[i];
    GetTheKeyedObjectFactory()->erase(key);
    for (auto i = loadmap.begin(); i != loadmap.end(); i++)
      if ((*i)->getkey() == key)
      {
        loadmap.erase(i);
        break;
      }
  }
}

void *
DynamicLibrary::GetSym(string symbol)
{
  void *p = dlsym(handle, symbol.c_str());
  if (!p) 
    std::cerr << "unable to load " << symbol << " from " << name << ": " << dlerror() << "\n";
  return p;
}

DynamicLibraryManager::DynamicLibraryManager()
{
  DynamicLibrary::RegisterClass();
}

void
DynamicLibraryManager::Register()
{
  FlushMsg::Register();
};

DynamicLibraryP
DynamicLibraryManager::Load(string name)
{
  for (auto i : loadmap)
    if (i->GetName() == name)
    {
      i->Ref();
      return i;
    }

  DynamicLibraryP dlp = DynamicLibrary::NewP();
  dlp->SetName(name);
  dlp->Commit();

  return dlp;
}

void
DynamicLibraryManager::Show()
{
  for (auto i = loadmap.begin(); i != loadmap.end(); i++)
    std::cerr << (*i)->GetName() << " " << (*i).use_count() << "\n";
}

}
