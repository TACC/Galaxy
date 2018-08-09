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

/*! \file Threading.h 
 * \brief manages threading including isolated threads and a pool of ganged threads
 */

#include <iostream>
#include <pthread.h>
#include <vector>
#include <string>
#include <list>
#include <future>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "Application.h"
#include "Events.h"

namespace gxy
{

//! manages all threads.   Gives access to per-thread data for each thread
/*! \ingroup framework
 */

class ThreadManager
{
private:

  struct TLS
  {
    TLS(int i, pthread_key_t k, std::string n, void *(*s)(void *), void *a);
    ~TLS();

    pthread_key_t key;
		int 					index;
    void*         (*start)(void *);
    void*         arg;
    long          tid;
    std::string   name;
		EventTracker  events;
  };

  static void DTOR(void *v) { TLS *foo = (TLS *)v; delete foo; }

  static void *START(void *v)
  {
    TLS *tls = (TLS *)v;
    tls->tid = ((long)pthread_self() & 0xffff);
    pthread_setspecific(tls->key, (void *)tls);
    return tls->start(tls->arg);
  }

  TLS *get_tls() { return (TLS *)pthread_getspecific(thr_id_key); }

  pthread_key_t thr_id_key;
  pthread_mutex_t lock;
	int index;

	EventTracker events; // event tracker for main thread and other non-managed threads 

public:
  ThreadManager();
  ~ThreadManager();

	//! Get argument passed to the current thread in its creation
  void* get_arg() { return get_tls()->arg; }

	//! Get the name of the current thread
  std::string get_name() { return get_tls()->name; }

	//! Get the index of the current thread
  int get_index() { return get_tls()->index; }

	//! Get the event handler of the current thread or the manager's thread if not a managed thread
  EventTracker* get_events() {
		if (get_tls() == NULL)
			return &events;
		else
			return &get_tls()->events;
	}

  //! create a new thread. 
	/*! Corresponds to pthread_create
   * \param name name assigned to current thread
	 * \param tid place to put the threads tid (see pthread_create)
	 * \param attr attributes for thread generation (see pthread_create)
	 * \param start_routine thread routine (see pthread_create)
	 * \param arg argument for thread routine (see pthread_create)
	 */
  int create_thread(std::string name, pthread_t *tid, const pthread_attr_t *attr, void *(*start_routine) (void *), void *arg);
};

class ThreadPool;

//! a prioritized task to be handled by a thread from the pool
/*! \ingroup framework
 * \sa ThreadPool
 */
class ThreadPoolTask
{
public:
	//! create a task with the given priority (larger int == higher priority)
	ThreadPoolTask(int p = 0) : priority(p) {};
	virtual ~ThreadPoolTask() {}; //!< default destructor
	virtual int work() = 0; //!< work to be performed by thread (must be defined by derived class) 
	virtual int get_priority() { return priority; } //!< return the priority of this task (larger int == higher priority)

	//! return the std::future signal for this thread
	std::future<int> get_future() { return p.get_future(); }

private:
	std::promise<int> p;
	friend class ThreadPool;
	int priority;
};

//! manages a pool of threads to handle priority-ranked tasks
/*! \ingroup framework 
 * 
 * Basic pattern is to create a threadpool:
 * ```
 *    ThreadPool threadpool(# of threads);
 * ```
 * Then add tasks which are subclasses of ThreadPoolTask, implementing
 * the work() procedure.   Give AddTask a pointer to the task; it'll delete
 * the task when its finished.
 * ```
 *		for (i = 0; i < nTasks; i++)
 *  		threadpool.AddTask(myTask[i])
 * ```
 * Then you can wait for all the tasks that have been added to the task queue
 * (since its creation) to finish:
 * ```
 *    threadpool.Wait()
 * ```
 * Or you can group up a bunch of tasks and wait for *them* to be processed:
 * ```
 *    std::vector< std::future<int> > task_list;
 *
 *		for (i = 0; i < nTasks; i++)
 *  		task_list.emplace_back(threadpool.AddTask(myTask[i]));
 *
 *    for (auto& t : task_list)
 *			t.get();
 * ```
 * In fact, `work()` should return an int, and the `t.get()` call should return
 * the result of the corresponding work unit
 * \sa ThreadPoolTask
 */
class ThreadPool
{
private:
	static void* thread(void *d);

public:
	enum PoolEventType { WAKE, START, FINISH };

	//! construct a thread pool with `n` threads
	ThreadPool(int n);

	//! destroy this thread pool
	~ThreadPool();

	//! return the highest priority task (in case of ties, selects the first in the task queue)
	virtual ThreadPoolTask *ChooseTask()
	{
		std::list<ThreadPoolTask *>::iterator besti = task_list.begin();
		int bestp = (*besti)->get_priority();

		for (std::list<ThreadPoolTask *>::iterator it = task_list.begin(); it != task_list.end(); ++it)
		{
			int p = (*it)->get_priority();
			if (p > bestp)
			{
				besti = it;
				bestp = (*it)->get_priority();
			}
		}

		ThreadPoolTask *task = (*besti);
		task_list.erase(besti);

		return task;
	}

	//! add a task to to the thread pool's queue
	std::future<int> AddTask(ThreadPoolTask *task)
	{
		std::future<int> f = task->p.get_future();

		pthread_mutex_lock(&lock);
		task_list.push_back(task);
		pthread_cond_signal(&wait);
		pthread_mutex_unlock(&lock);

		return f;
	}

	//! wait until all tasks in the queue have been processed
	void Wait()
	{
		pthread_mutex_lock(&lock);
		while(task_list.size())
			pthread_cond_wait(&wait_for_done, &lock);
		pthread_mutex_unlock(&lock);
	}

	//! add an event noting an event (wake, start, finish task)
	void PoolEvent(PoolEventType e)
	{
		GetTheEventTracker()->Add(new PoolThreadEvent(e));
	}

private:
	std::vector<pthread_t> thread_ids;
	std::list<ThreadPoolTask *> task_list;

	bool stop;

	pthread_mutex_t lock;
	pthread_cond_t wait;
	pthread_cond_t wait_for_done;

  class PoolThreadEvent : public Event
  {
  public:
    PoolThreadEvent(PoolEventType e) : pe(e) {}

  protected:
    void print(std::ostream& o)
    {
      Event::print(o);

			switch(pe)
			{
				case WAKE:   o << "pool thread wakeup"; break;
				case START:  o << "pool thread start task"; break;
				case FINISH: o << "pool thread finish task"; break;
			}
    }

  private:
    PoolEventType pe;
  };

};

} // namespace gxy
