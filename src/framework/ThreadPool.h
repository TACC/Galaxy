#include <stdlib.h>
#include <memory>
#include <iostream>
#include <pthread.h>
#include <vector>

using namespace std;
namespace pvol
{

class Thread;

class ThreadPool
{
public:
  enum ThreadState { STARTING, QUIT, RUNNING, IDLE, DONE };

  static const char *state_str(ThreadState s)
  {
    return s == STARTING ? "STARTING" :
           s == QUIT ? "QUIT" :
           s == RUNNING ? "RUNNING" :
           s == IDLE ? "IDLE" :
           s == DONE ? "DONE" : "????";
  }

  ThreadPool();
  ~ThreadPool();

  void Init(int nthreads);

  void Lock()   { pthread_mutex_lock(&lock); }
  void Unlock() { pthread_mutex_unlock(&lock); }
  void Wait()   { pthread_cond_wait(&cond, &lock); }
  void Signal() { pthread_cond_signal(&cond); }

  bool AllocateAndUnlock(vector<Thread*>&, int n);
  int  GetNumberOfThreadsAvailableAndLock();

  Thread* Allocate(bool wait = true);

  void ReturnThread(Thread*);
  void dump();

private:
  pthread_mutex_t lock;
  pthread_cond_t  cond;
  vector<Thread*> threads;
  vector<Thread*> idle_threads;
};

class ThreadTask
{
public: 
  virtual void operator()() { cout << "default task\n"; }
};

class Thread
{
public:
  Thread(ThreadPool *);
  ~Thread();

  void WaitToComplete();

  ThreadPool::ThreadState State() { return state; }

  void Lock()   { pthread_mutex_lock(&lock); }
  void Unlock() { pthread_mutex_unlock(&lock); }
  void Wait()   { pthread_cond_wait(&cond, &lock); }
  void Signal() { pthread_cond_signal(&cond); }
   
  void Run(ThreadTask*);
  int ID() { return (id); }

private:

  static void* thread_worker(void *p);

private:
  ThreadPool::ThreadState state;
  ThreadTask* threadTask;
  pthread_t tid;
  pthread_mutex_t lock;
  pthread_cond_t  cond;
  ThreadPool *pool;
  int id;
};

}

