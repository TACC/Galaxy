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

#include <fstream>
#include <sstream>

#include "Application.h"
#include "Threading.h"
#include "Events.h"

using namespace std;

namespace gxy
{

ThreadManager::TLS::TLS(int i, pthread_key_t k, std::string n, void *(*s)(void *), void *a) : index(i), key(k), name(n), start(s), arg(a)
{
}

ThreadManager::TLS::~TLS()
{
#if defined(GXY_EVENT_TRACKING)
  int rank = GetTheApplication()->GetTheMessageManager()->GetRank();
  fstream fs;
  stringstream fname;
  fname << "gxy_events" << "_" << rank << "_" << index;
  fs.open(fname.str().c_str(), fstream::out);
	fs << "thread name: " << name << "\n";
  events.DumpEvents(fs);
  fs.close();
#endif
}

ThreadManager::ThreadManager()
{
	pthread_key_create(&thr_id_key, DTOR);
	pthread_mutex_init(&lock, NULL);
  index = 0;
}

ThreadManager::~ThreadManager()
{
#if defined(GXY_EVENT_TRACKING)
  int rank = GetTheApplication()->GetTheMessageManager()->GetRank();
  fstream fs;
  stringstream fname;
  fname << "gxy_events" << "_" << rank << "_unmanaged";
  fs.open(fname.str().c_str(), fstream::out);
  fs << "thread name: unmanaged\n";
  events.DumpEvents(fs);
  fs.close();
#endif
	pthread_mutex_destroy(&lock);
}

int
ThreadManager::create_thread(std::string n, pthread_t *tid, const pthread_attr_t *a, void *(*s) (void *), void *arg)
{
	pthread_mutex_lock(&lock);
	TLS *tls = new TLS(index++, thr_id_key, n, s, arg);
	pthread_mutex_unlock(&lock);
	return pthread_create(tid, a, START, (void *)tls);
}

ThreadPool::ThreadPool(int n)
{
	stop = false;

	tChoose = tWait = tWork = 0;
	tStart = gettime();
	nPoolThreads = n;

	number_of_tasks = 0;

	pthread_mutex_init(&lock, NULL);
	pthread_cond_init(&wait, NULL);
	pthread_cond_init(&wait_for_done, NULL);

	for (int i = 0; i < n; i++)
	{
		pthread_t t;
		char name[256];
		sprintf(name, "pool_%d", i);
		if (GetTheApplication()->GetTheThreadManager()->create_thread(std::string(name), &t, NULL, thread, (void *)this))
		{
			std::cerr << "error creating thread pool" << std::endl;
			exit(1);
		}
		thread_ids.push_back(t);
	}
}

ThreadPool::~ThreadPool()
{
	pthread_mutex_lock(&lock);
	stop = true;
	pthread_cond_signal(&wait);
	pthread_mutex_unlock(&lock);

	for (std::vector<pthread_t>::iterator it = thread_ids.begin(); it != thread_ids.end(); ++it)
		pthread_join(*it, NULL);

	pthread_cond_destroy(&wait);
	pthread_mutex_destroy(&lock);
}

void *
ThreadPool::thread(void *d) 
{
	ThreadPool *pool = (ThreadPool *)d;

	register_thread(std::string("thread_pool"));

	pthread_mutex_lock(&pool->lock);

	while (! pool->stop)
	{
		double tWaitStart = pool->gettime();

		while ((! pool->stop) && (pool->GetNumberOfTasks() == 0))
			pthread_cond_wait(&pool->wait, &pool->lock);

		double tWaitEnd = pool->gettime();

		if (! pool->stop)
		{
			pool->PoolEvent(WAKE);

			double tChooseStart = pool->gettime();
			ThreadPoolTask *task = pool->ChooseTask();
			double tChooseEnd  = pool->gettime();

			pthread_mutex_unlock(&pool->lock);

			pool->PoolEvent(START);

			double tWorkStart = pool->gettime();
			task->p.set_value(task->work());
			double tWorkEnd = pool->gettime();
			delete task;

			pool->PoolEvent(FINISH);

			pthread_mutex_lock(&pool->lock);

			pool->stats(tWaitEnd - tWaitStart, tChooseEnd - tChooseStart, tWorkEnd - tWorkStart);

			pthread_cond_signal(&pool->wait_for_done);
		}
	}

	pthread_cond_signal(&pool->wait);
	pthread_mutex_unlock(&pool->lock);
	pthread_exit(NULL);
}

static int hw = 0;

ThreadPoolTask *
ThreadPool::ChooseTask()
{   
	// Search for either the appropriate list or the location to insert one
	//
	for (std::list<PriorityLevel *>::iterator it = priority_list.begin(); it != priority_list.end(); it++)
		if ((*it)->tasks.size() > 0)
		{
			ThreadPoolTask *tpt = (*it)->tasks.back();
			(*it)->tasks.pop_back();
			number_of_tasks --;
			return tpt;
		}

	std::cerr << "Choosing task from empty task list?";
	return NULL;
}

std::future<int> 
ThreadPool::AddTask(ThreadPoolTask *task)
{
	std::future<int> f = task->p.get_future();

	pthread_mutex_lock(&lock);

	std::deque<ThreadPoolTask*> *l = NULL;
	std::list<PriorityLevel *>::iterator it;

	// Search for either the appropriate list or the location to insert one
	//
	for (it = priority_list.begin(); it != priority_list.end(); it++)
	{
		if ((*it)->priority < task->get_priority())
			break;
		else if ((*it)->priority == task->get_priority())
		{
			l = &(*it)->tasks;
			break;
		}
	}

	// If no appropriate list, create one and insert it
	// 
	if (!l)
	{
			PriorityLevel *pl = new PriorityLevel(task->get_priority());
			priority_list.insert(it, pl);
			l = &(pl->tasks);
	}

	l->push_back(task);
	number_of_tasks ++;

	pthread_cond_signal(&wait);
	pthread_mutex_unlock(&lock);

	return f;
}

void 
ThreadPool::stats(double wait, double choose, double work)
{
	tWait += wait;
	tChoose += choose;
	tWork += work;
	
	double tNow = gettime();
	double tElapsed = tNow - tStart;
	if (tNow - tStart > 10)
	{
		std::cerr << "wait: " << ((tWait / nPoolThreads) / tElapsed) << 
								 " work: " << ((tWork / nPoolThreads) / tElapsed) << 
								 " choose: " << (tChoose / tElapsed) << 
								 " sum: " << (((tWait + tWork + tChoose) / nPoolThreads) / tElapsed) << 
								 "\n";

		tWait = tChoose = tWork = 0;
		tStart = tNow;
	}
}

}
