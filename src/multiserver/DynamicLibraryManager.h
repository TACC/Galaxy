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

/*! \file DynamicLibraryManager.h
 *  \brief DynamicLibrary and DynamicLibraryManager manage dynamic libraries
 *
 * Galaxy's MultiServer supports multiple "applications" that conneact
 * to a Galaxy server, interact with data there, and exit, leaving the
 * server running.  To do so, applications must install server-side
 * code - on each participating server - to actually work inside the
 * distributed server.   DynamicLibraryManager, instantiated in the
 * master node's process, allows each client to request that a dynamic
 * library, implementing the clients server-side functions, be available.

 * The manager maintains a list of loaded dynamic libraries (DynamicLibrary
 * objects) to avoid double-loading.  Each DL is referenced by everything
 * that requires it be loaded: the client application and any objects
 * of client-specific types.   The DL can only be removed when these
 * are all gone - the application itself for obvious reasons, and the
 * objects, since their deletion code exists in the DL and, otherwise,
 * a segfault will occur when their destrictor is called.
 * 
 * Whenever a client application terminates, the DLM is flushed to
 * remove any DLs that are not referenced outside the DynamicLibraryManager.
 */

#pragma once

#include <string>
#include <dlfcn.h>
#include <vector>
#include <memory>

#include "KeyedObject.h"

namespace gxy
{

KEYED_OBJECT_POINTER_TYPES(DynamicLibrary)

class DynamicLibrary : public KeyedObject
{
  KEYED_OBJECT(DynamicLibrary)

public:
  ~DynamicLibrary();
  void initialize() {}

  //! commit this object to the global registry across all processes
  virtual bool Commit();

  //! Set/Get the DL name
  void SetName(std::string s) { name = s; }
  std::string GetName() { return name; }

  //! Cause the library to be loaded on all server processes
  DynamicLibraryDPtr Load();

  //! Access a particular symbol from the DL
  void *GetSym(std::string);

  virtual int serialSize();
  virtual unsigned char *serialize(unsigned char *);
  virtual unsigned char *deserialize(unsigned char *);
  virtual bool local_commit(MPI_Comm);

private:
  void *handle;
  std::string name;

  class CommitMsg : public Work
  {
  public:
    CommitMsg(DynamicLibrary*);

    // defined in Work.h
    WORK_CLASS(CommitMsg, false);

  public:
    bool CollectiveAction(MPI_Comm c, bool isRoot);
  };

};

typedef std::shared_ptr<DynamicLibrary> DynamicLibraryDPtr;

class DynamicLibraryManager
{
public:
  DynamicLibraryManager();
  DynamicLibraryDPtr Load(std::string);

  static void Register();
  
  void Flush();
  void Show();

  //! LOCAL: add a DynamicLibrary object to the load map
  void post(DynamicLibraryDPtr dlp) { loadmap.push_back(dlp); }

  //! LOCAL: flush unneeded DynamicLibraries
  void flush(int, Key *);

private:
  std::vector<DynamicLibraryDPtr> loadmap;

  class FlushMsg : public Work
  {
  public:
    FlushMsg(std::vector<Key>);

    // defined in Work.h
    WORK_CLASS(FlushMsg, false);

  public:
    bool CollectiveAction(MPI_Comm c, bool isRoot);
  };

};

}
