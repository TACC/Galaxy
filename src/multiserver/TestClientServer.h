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
#include <sstream>

#include <string.h>
#include <pthread.h>

#include "Application.h"
#include "Datasets.h"
#include "MultiServerHandler.h"

namespace gxy
{

OBJECT_POINTER_TYPES(TestObject)

class TestObject : public KeyedDataObject
{
  KEYED_OBJECT_SUBCLASS(TestObject, KeyedDataObject)

  void SetDynamicLibrary(DynamicLibraryP d) { dlp = d; }

  void SetString(string s) { str = s; }
  string GetString() { return str; }

  virtual int serialSize()
  {
    return super::serialSize() + str.size() + 1;
  }

  virtual unsigned char *serialize(unsigned char *p)
  {
    p = super::serialize(p);
    memcpy(p, str.c_str(), str.size()+1);
    return p + str.size() + 1;
  }

  virtual unsigned char *deserialize(unsigned char *p)
  {
    p = super::deserialize(p);
    str = (char *)p;
    return p + str.size() + 1;
  }

public: 
  ~TestObject() { cerr << "TestObject dtor: " << str << "\n"; }

private:
  string str;
  DynamicLibraryP dlp;
};

class ShowDatasetsMsg : public Work
{
public:
  ShowDatasetsMsg(DatasetsP dsp) : ShowDatasetsMsg(sizeof(Key))
  {
    *(Key *)get() = dsp->getkey();
  }
  ~ShowDatasetsMsg() {}

  WORK_CLASS(ShowDatasetsMsg, true)

public:
  bool Action(int s);
};

class ShowKeyedObjectsMsg : public Work
{
public:
  ~ShowKeyedObjectsMsg() {}

  WORK_CLASS(ShowKeyedObjectsMsg, true)

  bool Action(int s);
};

class TestClientServer : public MultiServerHandler
{
public:
  static void init();
  bool handle(std::string line, std::string& reply);
};


}

