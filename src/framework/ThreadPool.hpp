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

/*! \file ThreadPool.hpp 
 * \brief manages a pool of threads to handle priority-ranked tasks
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


namespace gxy
{

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
	static void* thread(void *d) 
	{
		ThreadPool *pool = (ThreadPool *)d;

		pthread_mutex_lock(&pool->lock);

		while (! pool->stop)
		{
			while ((! pool->stop) && pool->task_list.empty())
				pthread_cond_wait(&pool->wait, &pool->lock);

			if (! pool->stop)
			{
				ThreadPoolTask *task = pool->ChooseTask();
				pthread_mutex_unlock(&pool->lock);

				task->p.set_value(task->work());
				delete task;

				pthread_mutex_lock(&pool->lock);
				pthread_cond_signal(&pool->wait_for_done);
			}
		}

		pthread_cond_signal(&pool->wait);
		pthread_mutex_unlock(&pool->lock);
		pthread_exit(NULL);
	}

public:
	//! construct a thread pool with `n` threads
	ThreadPool(int n) 
	{
		stop = false;

		pthread_mutex_init(&lock, NULL);
		pthread_cond_init(&wait, NULL);
		pthread_cond_init(&wait_for_done, NULL);

		for (int i = 0; i < n; i++)
		{
			pthread_t t;
			if (pthread_create(&t, NULL, thread, (void *)this))
			{
				std::cerr << "error creating thread pool" << std::endl;
				exit(1);
			}
			thread_ids.push_back(t);
		}
	}

	//! destroy this thread pool
	~ThreadPool()
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

	//! return the highest priority task (in case of ties, selects the first in the task queue)
	virtual ThreadPoolTask *ChooseTask()
	{
		std::list<ThreadPoolTask *>::iterator besti = task_list.begin();
		int bestp = (*besti)->get_priority();

		// int pknts[] = {0, 0, 0};
		for (std::list<ThreadPoolTask *>::iterator it = task_list.begin(); it != task_list.end(); ++it)
		{
			int p = (*it)->get_priority();
			// pknts[p] ++;
			if (p > bestp)
			{
				besti = it;
				bestp = (*it)->get_priority();
			}
		}

		// std::cerr << pknts[0] << " " << pknts[1] << " " << pknts[2] << ": " << bestp << "\n";

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

private:
	std::vector<pthread_t> thread_ids;
	std::list<ThreadPoolTask *> task_list;

	bool stop;

	pthread_mutex_t lock;
	pthread_cond_t wait;
	pthread_cond_t wait_for_done;
};

} // namespace gxy
