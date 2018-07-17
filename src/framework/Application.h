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

#include <pthread.h>
#include <sstream>
#include <string>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

#include "debug.h"
#include "Events.h"
#include "galaxy.h"
#include "KeyedObject.h"
#include "MessageManager.h"
#include "Work.h"

#include "ThreadPool.hpp"

#include "rapidjson/document.h"

namespace gxy
{

#define APP_PRINT(x)																				\
{																														\
  std::stringstream ss;																			\
	ss x;																											\
	GetTheApplication()->Print(ss.str());											\
}

#define APP_LOG(x)																					\
{																														\
  std::stringstream ss;																			\
	ss << my_gettid() << " " << my_threadname() << " " <<  __FUNCTION__ << ": " x;			\
	GetTheApplication()->Log(ss.str());												\
}

struct ClassTableEntry
{
  ClassTableEntry(std::string s);
	ClassTableEntry(const ClassTableEntry&);
  ~ClassTableEntry();
  const char *c_str();
  std::string *my_string;                                   
  int indx;
};

class Application;
extern Application* GetTheApplication();

class Application {

public:
  Application();
  Application(int *argc, char ***argv);
  ~Application();

  MessageManager *GetTheMessageManager() { return theMessageManager; }
  KeyedObjectFactory *GetTheKeyedObjectFactory() { return theKeyedObjectFactory; }

  ThreadPool *GetTheThreadPool() { return threadPool; }

	void Print(std::string);
	void Log(std::string);
	void DumpLog();

  void QuitApplication();
  void SyncApplication();

	void DumpEvents();

  int *GetPArgC() { return argcp; }
  char ***GetPArgV() { return argvp; }

  int GetSize() { return GetTheMessageManager()->GetSize(); }
  int GetRank() { return GetTheMessageManager()->GetRank(); }

  void Start(bool with_mpi = true);
  void Kill();
	void Wait();
  bool Running() { return !application_done; }

	void Pause() { GetTheMessageManager()->Pause(); }
	void Run() { GetTheMessageManager()->Run(); }

  Work *Deserialize(Message *msg);
  const char *Identify(Message *msg);

  int RegisterWork(std::string name, Work *(*f)(SharedP)) {
    int n = (*deserializers).size();
    (*deserializers).push_back(f);
		ClassTableEntry c(name.c_str());
		(*class_table).push_back(c);
    return n;
  }

  bool IsDoneSet() { return application_done; }
	pid_t get_pid() { return pid; }

	rapidjson::Document *OpenInputState(std::string s);
	rapidjson::Document *OpenOutputState();
	void SaveOutputState(rapidjson::Document *, std::string s);

private:
	std::vector<std::string> log;
	MessageManager *theMessageManager;
	KeyedObjectFactory *theKeyedObjectFactory;

  std::vector<Work *(*)(SharedP)> *deserializers;
  std::vector<ClassTableEntry> *class_table;

  ThreadPool *threadPool;

  bool application_done;

  int *argcp;
  char ***argvp;

	pid_t pid;

  pthread_mutex_t lock;
  pthread_cond_t cond;

	EventTracker eventTracker;

private:
	class QuitMsg : public Work
	{
	public:
		QuitMsg(){};
		WORK_CLASS(QuitMsg, true)

	public:
		bool CollectiveAction(MPI_Comm coll_comm, bool isRoot);
	};

	class DumpEventsMsg : public Work
	{
	public:
		DumpEventsMsg(){};
		WORK_CLASS(DumpEventsMsg, true)

	public:
		bool CollectiveAction(MPI_Comm coll_comm, bool isRoot)
		{
			MPI_Barrier(coll_comm);
			GetTheApplication()->DumpEvents();
			MPI_Barrier(coll_comm);
			return false;
		}
	};

	class SyncMsg : public Work
	{
	public:
		SyncMsg(){};
		WORK_CLASS(SyncMsg, true)

	public:
		bool CollectiveAction(MPI_Comm coll_comm, bool isRoot);
	};

	class PrintMsg : public Work
	{
		WORK_CLASS(PrintMsg, false);
	public:
		PrintMsg(std::string &);
		bool Action(int sender);
	};
};

extern long my_gettid();
extern const char *threadname_by_id(long l);
extern const char *my_threadname();
extern void register_thread(std::string s);

} // namespace gxy

