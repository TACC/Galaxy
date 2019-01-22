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

#include "KeyedObject.h"

namespace gxy
{

OBJECT_POINTER_TYPES(DynamicLibrary)

class DynamicLibrary : public KeyedObject
{
  KEYED_OBJECT(DynamicLibrary)

public:
  ~DynamicLibrary();

  void initialize()
  {
    refknt = 1;
  }

  //! commit this object to the global registry across all processes
  virtual void Commit();

  void SetName(std::string s) { name = s; }
  std::string GetName() { return name; }

  DynamicLibraryP Load();
  void *GetSym(std::string);

  void Ref() { refknt++; }
  void Deref() { refknt--; }

  bool IsUnloadable() { return refknt == 0; }

  virtual int serialSize();
  virtual unsigned char *serialize(unsigned char *);
  virtual unsigned char *deserialize(unsigned char *);
  virtual bool local_commit(MPI_Comm);

private:
  void *handle;
  int refknt;
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

typedef std::shared_ptr<DynamicLibrary> DynamicLibraryP;

class DynamicLibraryManager
{
public:
  DynamicLibraryManager();
  DynamicLibraryP Load(std::string);

  static void Register();
  
  void Flush();
  void Show();

  //! LOCAL: add a DynamicLibrary object to the load map
  void post(DynamicLibraryP dlp) { loadmap.push_back(dlp); }

  //! LOCAL: flush unneeded DynamicLibraries
  void flush(int, Key *);

private:
  std::vector<DynamicLibraryP> loadmap;

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
