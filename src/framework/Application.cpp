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


#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <dlfcn.h>

#include <iostream>
#include <sstream>
#include <fstream>
#include <map>

#include "Application.h"
#include "KeyedObject.h"
#include "Threading.h"
#include "Events.h"

#include "tbb/tbb.h"
#include "tbb/task_scheduler_init.h"

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
WORK_CLASS_TYPE(Application::LoadSOMsg)

static Application *theApplication = NULL;

int cte_indx = 0;

ClassTableEntry::ClassTableEntry(string s)
{
	my_string = new string(s.c_str());
	indx = cte_indx++;
}

ClassTableEntry::ClassTableEntry(const ClassTableEntry& o)
{
	
	indx = cte_indx++;
	my_string = new string(o.my_string->c_str());
}

ClassTableEntry::~ClassTableEntry()
{
	delete my_string;
}

const char *ClassTableEntry::c_str() { return my_string->c_str(); }

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

  int n_threads =  getenv("GXY_NTHREADS") ?  atoi(getenv("GXY_NTHREADS")) : 5;
  std::cerr << "Using " << n_threads << " threads in rendering thread pool." << std::endl;

  threadManager = new ThreadManager;
  threadPool = new ThreadPool(n_threads);

  class_table = new vector<ClassTableEntry>;
  deserializers =  new vector<Work *(*)(SharedP)>;
	theMessageManager = new MessageManager; 
	theKeyedObjectFactory = new KeyedObjectFactory; 

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
  LoadSOMsg::Register();

	KeyedObject::Register();
	KeyedObjectFactory::Register();

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
	delete class_table;
	delete deserializers;
	delete theMessageManager;
	delete theKeyedObjectFactory;
	delete threadManager;

  theApplication = NULL;
}

void Application::QuitApplication()
{
  GetTheKeyedObjectFactory()->Clear();

	QuitMsg q;
	q.Broadcast(true, true);
	Application::Wait();
}

void Application::SyncApplication()
{
	SyncMsg *q = new SyncMsg(0);
	q->Broadcast(true, true);
}

bool
Application::loadSO(string soname)
{
  if (shared_objects.find(soname) == shared_objects.end())
  {
    void *handle = dlopen(const_cast<char *>(soname.c_str()), RTLD_NOW);
    if (handle)
    {
      cerr << "initting " << soname << "\n";

      void *(*init)();
      init = (void *(*)()) dlsym(handle, const_cast<char *>("init"));
      init();
    }
    else
      std::cerr << "Error opening SO: " << dlerror() << "\n";

    shared_objects[soname] = handle;
    return true;
  }
  else
    return false;

}
   
void *Application::LoadSO(string soname)
{
  if (shared_objects.find(soname) == shared_objects.end())
  {
    LoadSOMsg *l = new LoadSOMsg(soname.size() + 1);
    unsigned char *ptr = (unsigned char *)l->get();
    memcpy(ptr, soname.c_str(), soname.size() + 1);

    l->Broadcast(true, true);
  }

  return shared_objects.find(soname)->second;
}

bool
Application::LoadSOMsg::CollectiveAction(MPI_Comm coll_comm, bool isRoot)
{
  GetTheApplication()->loadSO((char *)get());
  MPI_Barrier(coll_comm);
  return false;
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
	return (*deserializers)[msg->GetType()](msg->ShareContent());
}

const char *
Application::Identify(Message *msg)
{
	return (*class_table)[msg->GetType()].c_str();
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
Application::OpenInputState(string s)
{
  Document *doc = new Document();

  FILE *pFile = fopen (s.c_str() , "r");
  char buf[64];
  rapidjson::FileReadStream is(pFile,buf,64);
  doc->ParseStream<0>(is);
  fclose(pFile);

	return doc;
}

Document *
Application::OpenOutputState()
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
