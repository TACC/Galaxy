#include <iostream>
#include <pthread.h>
#include <vector>
#include <string>
#include <list>
#include <future>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

//
// Basic pattern is to create a threadpool:
//
//    ThreadPool threadpool(# of threads);
//
// Then add tasks which are subclasses of ThreadPoolTask, implementing
// the work() procedure.   Give AddTask a pointer to the task; it'll delete
// the task when its finished.
//
//		for (i = 0; i < nTasks; i++)
//  		threadpool.AddTask(myTask[i])
//
// Then you can wait for all the tasks that have been added to the task queue
// (since its creation) to finish:
// 
//    threadpool.Wait()
//
// Or you can group up a bunch of tasks and wait for *them* to be processed:
//
//    std::vector< std::future<int> > task_list;
//
//		for (i = 0; i < nTasks; i++)
//  		task_list.emplace_back(threadpool.AddTask(myTask[i]));
//
//    for (auto& t : task_list)
//			t.get();
//
// In fact, work() should return an int, and the 't.get()' call should return
// the result of the corresponding work unit
//

namespace pvol
{

class ThreadPool;

class ThreadPoolTask
{
public:
	ThreadPoolTask() {};
	virtual ~ThreadPoolTask() {};
	virtual int work() = 0;

	std::future<int> get_future() { return p.get_future(); }

private:
	std::promise<int> p;
	friend class ThreadPool;
};
	
class ThreadPool
{
private:
	static void* thread(void *d) 
	{
		ThreadPool *pool = (ThreadPool *)d;

		pthread_mutex_lock(&pool->lock);

		while (! pool->stop)
		{
			while ((! pool->stop) && (pool->task_list.size() == 0))
				pthread_cond_wait(&pool->wait, &pool->lock);

			if (! pool->stop)
			{
				ThreadPoolTask *task = pool->task_list.front();
				pool->task_list.pop_front();
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
				std::cerr << "error creating thread pool\n";
				exit(1);
			}
			thread_ids.push_back(t);
		}
	}

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

	std::future<int> AddTask(ThreadPoolTask *task)
	{
		std::future<int> f = task->p.get_future();

		pthread_mutex_lock(&lock);
		task_list.push_back(task);
		pthread_cond_signal(&wait);
		pthread_mutex_unlock(&lock);

		return f;
	}

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

}
