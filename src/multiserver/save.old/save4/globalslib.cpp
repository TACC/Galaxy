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

/*
*/

#include <string>
#include <map>
#include <unistd.h>
#include <sstream>
#include <time.h>
#include <dlfcn.h>
#include <pthread.h>

#include "Application.h"
#include "MultiServerHandler.h"
#include "MultiServerObject.h"

using namespace gxy;
using namespace std;

void brk() {}

namespace gxy
{
  OBJECT_POINTER_TYPES(TestObject)

  class TestObject : public KeyedObject, public MultiServerObject
  {
    KEYED_OBJECT(TestObject);

  public:
    ~TestObject() { cerr << GetTheApplication()->GetRank() << ": TestObject dtor\n"; }

    virtual int serialSize() { return super::serialSize() + s.size() + 1; }

    virtual unsigned char *serialize(unsigned char *ptr)
    { 
      ptr = super::serialize(ptr);
      memcpy(ptr, s.c_str(), s.size()+1); 
      return ptr + s.size() + 1;
    }

    virtual unsigned char *deserialize(unsigned char *ptr) 
    {
      ptr = super::deserialize(ptr);
      set_string((char *)ptr);
      return ptr + strlen((char *)ptr) + 1;
    }

    void set_string(string _s) { s = _s; }
    string get_string() { return s; }

  private:
    string s;
  };

  void TestObject::Register() { RegisterClass(); }

  KEYED_OBJECT_CLASS_TYPE(TestObject)

  pthread_mutex_t lck = PTHREAD_MUTEX_INITIALIZER;
  pthread_cond_t  w8 = PTHREAD_COND_INITIALIZER;

  class PrintTestObjectMsg : public Work
  {
  public:

    PrintTestObjectMsg(TestObjectP to) : PrintTestObjectMsg(sizeof(Key)) { *(Key *)get() = to->getkey(); }
    ~PrintTestObjectMsg() {}

    WORK_CLASS(PrintTestObjectMsg, true)

    bool Action(int s)
    { 
      int rank = GetTheApplication()->GetRank();
      int size = GetTheApplication()->GetSize();

      TestObjectP to = TestObject::GetByKey(*(Key *)get());
      std::cerr << rank << ": " << to->get_string() << "\n";

      if (rank != 0 && size > 1)
      {
        PrintTestObjectMsg m(to);
        m.Send((rank == (size-1)) ? 0 : rank + 1);
      }
      else
      {
        pthread_mutex_lock(&lck);
        pthread_cond_signal(&w8);
        pthread_mutex_unlock(&lck);
      }
    
      return false;
    }
  };

  WORK_CLASS_TYPE(PrintTestObjectMsg);

  class ShowKeyedObjectsMsg : public Work
  {
  public:
    ~ShowKeyedObjectsMsg() {}

    WORK_CLASS(ShowKeyedObjectsMsg, true)

    bool Action(int s)
    { 
      int rank = GetTheApplication()->GetRank();
      int size = GetTheApplication()->GetSize();

      std::cerr << "Rank " << rank << ":\n";
      GetTheApplication()->GetTheKeyedObjectFactory()->Dump();

      if (rank != 0)
      {
        ShowKeyedObjectsMsg m(1);
        m.Send((rank == (size-1)) ? 0 : rank + 1);
      }
      else
      {
        pthread_mutex_lock(&lck);
        pthread_cond_signal(&w8);
        pthread_mutex_unlock(&lck);
      }
    
      return false;
    }
  };

  WORK_CLASS_TYPE(ShowKeyedObjectsMsg);
}

void
ShowKeyedObjectTable()
{
  if (GetTheApplication()->GetSize() > 1)
  {
    pthread_mutex_lock(&lck);

    ShowKeyedObjectsMsg m(1);
    m.Send(1);

    pthread_cond_wait(&w8, &lck);
    pthread_mutex_unlock(&lck);
  }
  else
  {
    std::cerr << "Rank " << GetTheApplication()->GetRank() << ":\n";
    GetTheApplication()->GetTheKeyedObjectFactory()->Dump();
  }
}

void
ShowKeyedObject(TestObjectP& to)
{
  if (GetTheApplication()->GetSize() > 1)
  {
    pthread_mutex_lock(&lck);

    PrintTestObjectMsg m(to);
    m.Send(1);

    pthread_cond_wait(&w8, &lck);
    pthread_mutex_unlock(&lck);
  }
  else
  {
    std::cerr << "Rank " << GetTheApplication()->GetRank() << ": " << to->get_string() << "\n";
    GetTheApplication()->GetTheKeyedObjectFactory()->Dump();
  }
}

extern "C" void
init()
{
  TestObject::Register();
  PrintTestObjectMsg::Register();
  ShowKeyedObjectsMsg::Register();
}

extern "C" bool
server(MultiServerHandler *handler)
{
  bool client_done = false;
  while (! client_done)
  {
    char *buf; int sz;
    if (! handler->CRecv(buf, sz))
    {
      cerr << "receive failed\n";
      break;
    }
    free(buf);

    cerr << "received " << buf << "\n";

    stringstream ss(buf);
    string cmd;

    ss >> cmd;
    if (cmd == "add")
    {
      string name;
      ss >> name;
      TestObjectP to = TestObject::NewP();
      to->set_string(name);
      to->Commit();
      GetTheApplication()->SetGlobal(name, to);
    }
    else if (cmd == "show")
    {
      string name;
      ss >> name;
      TestObjectP to = TestObject::Cast(GetTheApplication()->GetGlobal(name));
      if (to)
        ShowKeyedObject(to);
      else
        cerr << "no such object: " << name << "\n";
    }
    else if (cmd == "libs")
      GetTheApplication()->GetTheDynamicLibraryManager()->Show();
    else if (cmd == "break")
      brk();
    else if (cmd == "drop")
    {
      string name;
      ss >> name;
      TestObjectP to = TestObject::Cast(GetTheApplication()->GetGlobal(name));
      if (to)
      {
        cerr << "dropping " << name << "\n";
        GetTheApplication()->DropGlobal(name);
      }
      else
        cerr << "no such object: " << name << "\n";
    }
    else if (cmd == "q" || cmd == "quit")
    {
      client_done = true;
    }
    else
      cerr << "huh?\n";

    std::string ok("ok");
    handler->CSend(ok.c_str(), ok.length()+1);
  }

  return true;
}

