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

#if 0
#include <stdlib.h>
#include <unistd.h>
#include <iostream>

#include "ThreadPool.h"

using namespace std;
namespace pvol
{

namespace gxy
{
static int nxt_thread = 0;

// Thread - Owns a pthread.   Ctor starts the thread, waits for the
// thread to indicate its ready.  Always owned by a pool

Thread::Thread(ThreadPool* tp)
{
  id = nxt_thread++;

  if (pthread_mutex_init(&lock, NULL)) cerr << "error creating\n";
  if (pthread_cond_init(&cond, NULL)) cerr << "error creating\n";

  pool = tp;

  Lock();
  state = ThreadPool::STARTING;

  if (pthread_create(&tid, NULL, thread_worker, (void *)this))
  {
    cerr << "error creating thread\n";
    exit(0);
  }

  while (state == ThreadPool::STARTING)
    Wait();

  if (state != ThreadPool::IDLE)
  {
    cerr << "Not idle in Thread ctor\n";
    exit(0);
  }
}

// Can't delete a thread if its not IDLE.  Signal pthread to quit,
// wait until it does.

Thread::~Thread()
{
  if (state != ThreadPool::IDLE)
     cerr << "~Thread: not IDLE\n";

  state = ThreadPool::QUIT;
  Signal();
  Unlock();

  pthread_join(tid, NULL);

  std::cerr << "thread " << id << ": " << pthread_self() << " destroyed\n";

  pthread_mutex_destroy(&lock);
  pthread_cond_destroy(&cond);
}

// thread_worker is the pthread proc.  On starting, it indicates
// its ready and waits for something to do.   When a task is ready
// it goes into the RUNNING state and runs the task.   When the task
// returns it enters the IDLE state, returns itself to the pool's
// idle list, and signals the pool (in case the pool is waiting for a
// free thread to allocate. Note that the lock is held when the thread is busy.

void * 
Thread::thread_worker(void *p)
{
  Thread *me = (Thread *)p;

  me->Lock();
  me->state = ThreadPool::IDLE;
  me->Signal();

  std::cerr << "thread " << me->id << " at " << std::hex << ((long)me) << " tid " << pthread_self() << " worker started\n";

  while (true)
  {
    while (me->state == ThreadPool::IDLE)
      me->Wait();

    // Have I been signalled to go away by Thread dtor?

    if (me->state == ThreadPool::QUIT)
      break;

    else if (me->state != ThreadPool::STARTING)
    {
      std::cerr << "thread state error\n";
      exit(0);
    }

    me->state = ThreadPool::RUNNING;
    // me->Signal();
    me->Unlock();

    (*me->threadTask)();
    delete me->threadTask;
    me->threadTask = NULL;

    me->Lock();
    me->state = ThreadPool::IDLE;
    me->pool->ReturnThread(me);
    me->Signal();
  }

  me->state = ThreadPool::DONE;
  me->Unlock();

  std::cerr << "thread " << me->id << ": " << pthread_self() << " worker exits\n";
  
  pthread_exit(NULL);
}

void
Thread::Run(ThreadTask* tp)
{
  Lock();
  if (state != ThreadPool::IDLE)
  {
    cerr << "Thread (" << id << ") Run called when state != IDLE (" 
         << ThreadPool::state_str(state) << ")\n";

    if (state != ThreadPool::IDLE)
    {
      cerr << "XXXXX" << ThreadPool::state_str(state) << "XXXX\n";
      exit(0);
    }
    else
      cerr << "nevermind\n";
  }

  threadTask = tp;
  state = ThreadPool::STARTING;
  Signal();
  /// while (state == ThreadPool::STARTING)
    /// Wait();
  Unlock();
}

ThreadPool::ThreadPool() 
{
  if (pthread_mutex_init(&lock, NULL)) cerr << "error creating\n";
  if (pthread_cond_init(&cond, NULL)) cerr << "error creating\n";
}
  
void
ThreadPool::Init(int nthreads)
{
  Lock();
  for (int i = 0; i < nthreads; i++)
  {
    // Push that many threads onto both the 
    // vector used to maintain ownership and
    // the vector of available threads.  Threads
    // know their threadpool so they can return 
    // themselves to the pool when they are done.

    Thread *t = new Thread(this);
    idle_threads.push_back(t);
    threads.push_back(t);
  }
  Unlock();
}

// Avail tells you how many threads are currently available in the
// pool.   If you use this you'd better lock the thread pool so 
// the threads don't get allocated out from under you.

int
ThreadPool::GetNumberOfThreadsAvailableAndLock()
{
  Lock();
  return idle_threads.size();
}

bool
ThreadPool::AllocateAndUnlock(vector<Thread*>& v, int n)
{
  if (n > idle_threads.size())
  {
    cerr << "can't satisfy request for " << n << " threads\n";
    return false;
  }

  for (int i = 0; i < n; i++)
  {
    v.push_back(idle_threads.back());
    idle_threads.pop_back();
  }

  Unlock();

  return true;
}

// Allocate a single thread.   The pool is locked and unlocked internally
// so a caller can simply:
//
// ThreadP tp = pool->Allocate()
// tp->Run(task)
//
// This is fire-and-forget.  If you want to know when its done, use a ThreadSet.
// Note that the thread will always be owned by the pool and by the caller (initially,
// at least)

Thread*
ThreadPool::Allocate(bool wait)
{
  Thread *t = nullptr;

  Lock();

  if (wait)
  {
    while (idle_threads.size() == 0)
    {
      Wait();
    }
  }

  if (idle_threads.size() > 0)
  {
    t = idle_threads.back();
    idle_threads.pop_back();
  }

  Unlock();
  return t;
}

void
ThreadPool::ReturnThread(Thread* t)
{
  Lock();
  idle_threads.push_back(t);
  Signal();
  Unlock();
}

void
ThreadPool::dump()
{
  usleep(1000000);
  cerr << "IDLE\n";
  for (auto t : idle_threads)
    cerr << t->ID() << "\n";
  cerr << "ALL\n";
  for (auto t : threads)
    cerr << t->ID() << "\n";
}

ThreadPool::~ThreadPool()
{
  for (auto t : threads)
    delete t;
  pthread_mutex_destroy(&lock);
  pthread_cond_destroy(&cond);
}
}
#endif
}

