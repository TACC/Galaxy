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

#include <iostream>
#include <vector>
#include <sstream>

using namespace std;

#include <string.h>
#include <pthread.h>

#include "TestClientServer.h"

namespace gxy
{

extern "C" void
init()
{
  TestClientServer::init();
}

extern "C" MultiServerHandler *
new_handler(SocketHandler *sh)
{
  return new TestClientServer(sh);
}

static pthread_mutex_t lck = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  w8 = PTHREAD_COND_INITIALIZER;

OBJECT_CLASS_TYPE(TestObject)
WORK_CLASS_TYPE(ShowDatasetsMsg)
WORK_CLASS_TYPE(ShowKeyedObjectsMsg)

bool 
ShowKeyedObjectsMsg::Action(int s)
{ 
  int rank = GetTheApplication()->GetRank();
  int size = GetTheApplication()->GetSize();

  std::cerr << "Rank " << rank << ":\n";
  GetTheApplication()->GetTheObjectFactory()->Dump();

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


bool 
ShowDatasetsMsg::Action(int s)
{ 
  int rank = GetTheApplication()->GetRank();
  int size = GetTheApplication()->GetSize();
  
  DatasetsDPtr dsp = Datasets::GetByKey(*(Key *)get());
  if (! dsp)
  { 
    cerr << "Process " << rank << ": No datasets object?\n";
    pthread_mutex_lock(&lck);
    pthread_cond_signal(&w8);
    pthread_mutex_unlock(&lck);
  }
  else
  { 
    if (rank == 0)
    {
      vector<string> ds_names = dsp->GetDatasetNames();

      cerr << "Process 0: name, key, string:\n";
      for (auto i : ds_names)
      { 
        TestObjectDPtr top = TestObject::Cast(dsp->Find(i));
        cerr << i << " " << top->getkey() << " " << top->GetString() << "\n";
      }
    }
    else
    {
      cerr << "Process " << rank << ": key, string:\n";
      for (int i = 0; i < dsp->get_number_of_keys(); i++)
      {
        Key k = dsp->get_key(i);
        TestObjectDPtr top = TestObject::GetByKey(k);
          cerr << k << " " << top->GetString() << "\n";
      }
    }

    if (rank == (size-1))
    { 
      pthread_mutex_lock(&lck);
      pthread_cond_signal(&w8);
      pthread_mutex_unlock(&lck);
    }
    else
    { 
      ShowDatasetsMsg tm(dsp);
      tm.Send(rank + 1);
    }
  }
  
  return false;
}

void
TestClientServer::init()
{ 
  TestObject::RegisterClass();
  ShowDatasetsMsg::Register();
  ShowKeyedObjectsMsg::Register();
}

bool
TestClientServer::handle(string line, string& reply)
{
  DatasetsDPtr theDatasets = Datasets::Cast(MultiServer::Get()->GetGlobal("global datasets"));
  if (! theDatasets)
  {
    theDatasets = Datasets::NewDistributed();
    MultiServer::Get()->SetGlobal("global datasets", theDatasets);
  }

  stringstream ss(line);
  string cmd;
  ss >> cmd;

  if (cmd == "create") 
  {
    string name;
    ss >> name;

    TestObjectDPtr to = TestObject::NewDistributed();
    to->SetString(name);
    to->Commit();

    theDatasets->Insert(name, to);
    theDatasets->Commit();

    reply = string("ok created ") + name;
    return true;
  }
  else if (cmd == "showds")  
  {
    ShowDatasetsMsg sd(theDatasets);
    sd.Send(0);
    reply = "ok";
    return true;
  }
  else if (cmd == "showko")
  {
    ShowKeyedObjectsMsg sko(1);
    sko.Send(0);
    reply = "ok";
    return true;
  }
  else if (cmd == "reset")
  {
    string name, newstring;
    ss >> name >> newstring;

    TestObjectDPtr to = TestObject::Cast(theDatasets->Find(name));

    if (! to)
    {
      reply = string("error ") + name + " hasn't been created\n";
      return true;
    }
    else
    {
      to->SetString(newstring);
      to->Commit();

      stringstream ss;
      ss << "ok " << name << " new string: " << newstring;
      reply = ss.str();
      return true;
    }
  }
  else if (cmd == "drop")
  {
    string name;
    ss >> name;

    TestObjectDPtr to = TestObject::Cast(theDatasets->Find(name));

    if (! to)
    {
      reply = string("error ") + name + " hasn't been created\n";
      return true;
    }
    else
    {
      theDatasets->DropDataset(name);
      theDatasets->Commit();

      ss << "ok " << name << " dropped";
      reply = ss.str();
      return true;
    }
  }
  else
    return false;
}

}
