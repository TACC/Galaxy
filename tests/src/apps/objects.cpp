// ========================================================================== //
// Copyright (c) 2014-2019 The University of Texas at Austin.                 //
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
This test demonstrates the lifetime of Galaxy objects.   Galaxy
objects are managed using std::shared_ptr.  Any Galaxy object -
that is, that eventually derive from GalaxyObject - has an accompanying
type for its shared pointer - for example, the Galaxy Camera object
(see Camera.h in renderer) defines GalaxyP - a shared pointer to a
Camera object.  Galaxy objects are created using NewP - a static
class method for each class.

Many Galaxy objects derive from KeyedObject, a subclass of GalaxyObject,
that implements objects that exist on all the Galaxy processes.
When a KeyedObject is created, it is assigned a Key as an identifier.
The object creation then issues a broadcast message that causes all
processes to create a corresponding local object, and associates
it with the given key.  Applications set state into the root process'
object, then call Commit to update all the server processes. Each
KeyedObject must implement serialSize, serialize and deserialize
methods to support this update.

In addition to whatever scoped references to an object exist, the
Application itself maintains a reference to the local object on
each process.  This enables *dependent* objects to be associated 
with the primary object by key.   For more information about this
please see the comments in KeyedObject.h

In this example, a new subclass of KeyedObject is defined.  A custom
message (PrintTestObjectMsg) is defined to pass a request to each
process (in a round-the-world fashion) causing each to print its
the local object's contents.   A second custom message (ShowKeyedObjectsMsg)
causes each process to list the objects owned by the Application, again
in a round-the-world fashion.

Initially two instances of the TestObject are created, and each' 
content is printed by issuing the PrintTestObjectMsg on each. Then
the objects owned by each process' Application are shown by issuing
ShowKeyedObjectsMsg.   Initially, each process owns two local 
correspondents of the two TestObjects.  Then the second TestObject is
deleted. One destructor method is called on each process,
and ShowKeyedObjectsMsg shows only one TestObject on each node.
The Appliucation then exits, and each of these remaining objects is 
destroyed, and none remain.
*/

#include <string>
#include <unistd.h>
#include <sstream>
#include <time.h>
#include <dlfcn.h>
#include <pthread.h>

#include "Application.h"

using namespace gxy;
using namespace std;

int mpiRank = 0, mpiSize = 1;

#include "Debug.h"

namespace gxy
{
  OBJECT_POINTER_TYPES(TestObject)

  class TestObject : public KeyedObject
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

      if (rank != 0)
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
  if (mpiSize > 1)
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
  if (mpiSize > 1)
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

using namespace gxy;

void
syntax(char *a)
{
  cerr << "syntax: " << a << " [options] " << endl;
  cerr << "options:\n";
  cerr << "  -D[which]     run debugger in selected processes.  If which is given, it is a number or a hyphenated range, defaults to all" << endl;

  exit(1);
}

int main(int argc,  char *argv[])
{
  char *dbgarg;
  bool dbg = false;
  bool atch = false;

  std::cerr << "pid: " << getpid() << "\n";
  
  Application theApplication(&argc, &argv);

  theApplication.Start();

  for (int i = 1; i < argc; i++)
  {
    if (!strncmp(argv[i],"-D", 2)) dbg = true, atch = false, dbgarg = argv[i] + 2;
    else syntax(argv[0]);
  }

  mpiRank = theApplication.GetRank();
  mpiSize = theApplication.GetSize();

  Debug *d = dbg ? new Debug(argv[0], atch, dbgarg) : NULL;

  theApplication.Run();

  TestObject::Register();
  PrintTestObjectMsg::Register();
  ShowKeyedObjectsMsg::Register();

  if (mpiRank == 0)
  {
    {
      TestObjectP to1 = TestObject::NewP();
      to1->set_string("test object 1");
      to1->Commit();

      TestObjectP to2 = TestObject::NewP();
      to2->set_string("test object 2");
      to2->Commit();

      std::cerr << "Showing object 1\n";
      ShowKeyedObject(to1);

      std::cerr << "Showing object 2\n";
      ShowKeyedObject(to2);

      std::cerr << "Object tables:\n";
      ShowKeyedObjectTable();
      // getchar();

      to2 = nullptr;  // should delted the object

      std::cerr << "After Deleting to2\n";
      ShowKeyedObjectTable();
      // getchar();
    }

    theApplication.QuitApplication();
    theApplication.Wait();

    std::cerr << "at exit... ? \n";
    // getchar();
  }
  else
    theApplication.Wait();
}
