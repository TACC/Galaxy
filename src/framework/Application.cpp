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


#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <dlfcn.h>

#include <iostream>
#include <sstream>
#include <fstream>
#include <stdexcept>
#include <map>

#include "Application.h"
#include "KeyedObjectMap.h"
#include "Threading.h"
#include "Events.h"

// #include "tbb/tbb.h"

#include "rapidjson/document.h"
#include "rapidjson/filereadstream.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"

using namespace rapidjson;
using namespace std;

namespace gxy
{

WORK_CLASS_TYPE(Application::QuitMsg)
WORK_CLASS_TYPE(Application::SyncMsg)
WORK_CLASS_TYPE(Application::PrintMsg)

static Application *theApplication = NULL;

Application *
GetTheApplication()
{
  return theApplication;
}

Application::Application(int *a, char ***b) : Application()
{
  argcp = a;
  argvp = b;
}

Application::Application()
{
  theApplication = this;

  quitting = false;

  int n_threads =  getenv("GXY_NTHREADS") ?  atoi(getenv("GXY_NTHREADS")) : 5;
  std::cerr << "Using " << n_threads << " threads in rendering thread pool." << std::endl;

  threadManager = new ThreadManager;
  threadPool = new ThreadPool(n_threads);

  theMessageManager = new MessageManager; 

#if 0
  if (getenv("GXY_APP_NTHREADS"))
  {
    int nthreads = atoi(getenv("GXY_APP_NTHREADS"));
    tbb::task_scheduler_init init(nthreads);
    std::cerr << "using " << nthreads << " TBB threads for application." << std::endl;
  }
  else
  {
    int n = tbb::task_scheduler_init::default_num_threads();
    std::cerr << "using " << n << " TBB threads for application." << std::endl;
  }
#endif

  pid = getpid();

  register_thread("Application");

  application_done = false;

  argcp = 0;
  argvp = NULL;

  pthread_mutex_init(&lock, NULL);
  pthread_cond_init(&cond, NULL);

  pthread_mutex_lock(&lock);

  QuitMsg::Register();
  SyncMsg::Register();
  PrintMsg::Register();

  KeyedObject::Register();
  KeyedObjectMap::Register();

  pthread_mutex_unlock(&lock);
}

  
static pthread_mutex_t log_lock = PTHREAD_MUTEX_INITIALIZER;

void
Application::Log(string s)
{
  pthread_mutex_lock(&log_lock);

  log.push_back(s);

  pthread_mutex_unlock(&log_lock);
}

void
Application::DumpLog()
{
  if (log.size() > 0)
  {
    std::stringstream fname;
    int rank = GetTheMessageManager()->GetRank();
    fname << "gxy_log_" << rank;

    std::fstream fs;
    fs.open(fname.str().c_str(), std::fstream::out | std::fstream::app);

    for (auto s: log)
      fs << s << "\n";

    fs.close();
  }
}

void
Application::Print(std::string s)
{
  Application::PrintMsg msg(s);
  msg.Send(0);
}

Application::~Application()
{
  DumpLog();
  
  pthread_mutex_unlock(&lock);

  delete threadPool;
  delete theMessageManager;
  delete threadManager;

  theApplication = NULL;
}

void Application::QuitApplication()
{
  GetTheKeyedObjectMap()->Clear();

  quitting = true;

  QuitMsg q;
  q.Broadcast(true, true);

  Application::Wait();
}

void Application::SyncApplication()
{
  SyncMsg *q = new SyncMsg(0);
  q->Broadcast(true, true);
}

void Application::Start(bool with_mpi)
{
  GetTheMessageManager()->Start(with_mpi);
}

void Application::Kill()
{
  pthread_mutex_lock(&lock);
  application_done = true;
  pthread_cond_broadcast(&cond);
  pthread_mutex_unlock(&lock);
}

bool
Application::QuitMsg::CollectiveAction(MPI_Comm coll_comm, bool isRoot)
{
  return true;
}

bool
Application::SyncMsg::CollectiveAction(MPI_Comm coll_comm, bool isRoot)
{
  MPI_Barrier(coll_comm);
  return false;
}

Work *
Application::Deserialize(Message *msg)
{
  return deserializers[msg->GetType()](msg->ShareContent());
}

const char *
Application::Identify(Message *msg)
{
  return class_table[msg->GetType()].c_str();
}

void
Application::Wait()
{
  while (! application_done)
    pthread_cond_wait(&cond, &lock);

  pthread_mutex_unlock(&lock);
}

Application::PrintMsg::PrintMsg(string &str) : Application::PrintMsg::PrintMsg(str.length() + 1)
{
  unsigned char *ptr = (unsigned char *)get();
  memcpy(ptr, str.c_str(), str.length() + 1);
}

bool 
Application::PrintMsg::Action(int sender)
{
  unsigned char *ptr = (unsigned char *)get();
  std::cerr << sender << ": " << (char *)ptr << "\n";
  return false;
}
  
Document *
Application::OpenJSONFile(string s)
{
  Document *doc = new Document();

  ifstream ifs(s);
  if (ifs)
  {
    stringstream ss;
    ss << ifs.rdbuf();

    if (doc->Parse<0>(ss.str().c_str()).HasParseError())
    {
      std::cerr << "Error parsing JSON file: " << s << "\n";
      delete doc;
      return NULL;
    }
  }
  else
  {
    std::cerr << "Error opening JSON file: " << s << "\n";
    delete doc;
    return NULL;
  }
  
  return doc;
}

Document *
Application::NewJSONDocument()
{
  Document *doc = new Document;
  doc->Parse("{}");
  return doc;
}

void
Application::SaveOutputState(Document *doc, string s)
{
  StringBuffer sbuf;
  PrettyWriter<StringBuffer> writer(sbuf);
  doc->Accept(writer);

  ofstream ofs;
  ofs.open(s.c_str(), ofstream::out);
  ofs << sbuf.GetString() << "\n";
  ofs.close();
}

pthread_mutex_t threadtable_lock = PTHREAD_MUTEX_INITIALIZER;
map<long, string> thread_map;

long
my_gettid()
{
  return ((long)pthread_self() & 0xffff);
}

const char *
my_threadname()
{
  if (thread_map.find(my_gettid()) == thread_map.end())
    return "this thread has no name";
  else
    return thread_map[my_gettid()].c_str();

}

void
register_thread(string s)
{
  pthread_mutex_lock(&threadtable_lock);
  thread_map[my_gettid()] = s;
  pthread_mutex_unlock(&threadtable_lock);
}

} // namespace gxy
