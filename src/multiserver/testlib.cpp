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
#include <vector>
#include <sstream>
using namespace std;

#include <string.h>
#include <pthread.h>

#include "Application.h"
#include "Datasets.h"
#include "MultiServer.h"
#include "MultiServerSocket.h"


#include "rapidjson/filereadstream.h"
using namespace rapidjson;

namespace gxy
{

OBJECT_POINTER_TYPES(TestObject)

class TestObject : public KeyedObject
{
  KEYED_OBJECT(TestObject)

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
};

OBJECT_CLASS_TYPE(TestObject)

class TestMsg : public Work
{
public:
  TestMsg(TestObjectP t) : TestMsg(sizeof(Key))
  {
    *(Key *)get() = t->getkey();
  }
  ~TestMsg() {}

  WORK_CLASS(TestMsg, true)

public:
  bool CollectiveAction(MPI_Comm s, bool isRoot)
  {
    TestObjectP to = TestObject::GetByKey(*(Key *)get());
    if (to)
      APP_PRINT(<< "to is " << to->GetString())
    else
      APP_PRINT(<< "to is NULL")

    return false;
  }
};

WORK_CLASS_TYPE(TestMsg)

}

using namespace gxy;

extern "C" void
init()
{
  extern void InitializeData();
  InitializeData();

  TestObject::RegisterClass();
  TestMsg::Register();
}

extern "C" bool
server(MultiServer *srvr, MultiServerSocket *skt)
{
  bool client_done = false;
  TestObjectP to;
  int knt = 0;

  while (! client_done)
  {
    char *buf; int sz;
    if (! skt->CRecv(buf, sz))
    {
      cerr << "receive failed\n";
      break;
    }

    cerr << "received " << buf << "\n";

    if (!strcmp(buf, "c"))
    {
      char buf[256]; 
      sprintf(buf, "C %d", knt++);

      to = TestObject::NewP();
      to->SetString(buf);
      to->Commit();

      string ok("ok");
      skt->CSend(ok.c_str(), ok.length()+1);
    }
    else if (! strcmp(buf, "t"))
    {
      if (! to)
        cerr << "to hasn't been created\n";
      else
      {
        cerr << "to key: " << to->getkey() << "\n";
        TestMsg tm(to);
        tm.Broadcast(true, true);
      }
      string ok("ok");
      skt->CSend(ok.c_str(), ok.length()+1);
    }
    else if (! strcmp(buf, "r"))
    {
      if (! to)
      {
        cerr << "to is NULL\n";
      }
      else 
      {
        cerr << "was " << to->GetString() << "\n";

        char buf[256]; 
        sprintf(buf, "R %d", knt++);

        to->SetString(buf);
        to->Commit();

        cerr << "now " << to->GetString() << "\n";
      }
      string ok("ok");
      skt->CSend(ok.c_str(), ok.length()+1);
    }
    else if (! strcmp(buf, "d"))
    {
      if (! to)
      {
        cerr << "to is NULL\n";
      }
      else 
      {
        cerr << "was " << to->GetString() << "\n";
        to->Drop();
        to = NULL;
      }
      string ok("ok");
      skt->CSend(ok.c_str(), ok.length()+1);
    }
    else if (!strcmp(buf, "q"))
    {
      client_done = true;
      string ok("quit ok");
      skt->CSend(ok.c_str(), ok.length()+1);
    }
    else
    {
      cerr << "huh?\n";
      string ok("quit ok");
      skt->CSend(ok.c_str(), ok.length()+1);
    }

    free(buf);
  }

  return true;
}
