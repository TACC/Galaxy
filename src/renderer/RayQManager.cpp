#define LOGGING 0

#include <iostream>
#include <sstream>
#include <pthread.h>

#include "Application.h"
#include "Renderer.h"
#include "RayQManager.h"
#include "Rays.h"

using namespace std;

RayQManager *RayQManager::theRayQManager;
RayQManager *RayQManager::GetTheRayQManager() { return RayQManager::theRayQManager; }

#if defined(EVENT_TRACKING)

class ProcessRaysEvent : public Event
{
public:
  ProcessRaysEvent(int c, Key r) : count(c), rset(r) {};

protected:
  void print(ostream& o)
  {
    Event::print(o);
    o << "processing " << count << " rays for rset " << rset;
  }

private:
  int count;
  Key rset;
};

#endif

#if 0
unsigned long tacc_rdtscp(int *chip, int *core)
{
   unsigned long int x;
   unsigned a, d, c;
   __asm__ volatile("rdtscp" : "=a" (a), "=d" (d), "=c" (c));
    *chip = (c & 0xFFF000)>>12;
    *core = c & 0xFFF;
   return ((unsigned long)a) | (((unsigned long)d) << 32);;
}
#endif


class rp_ftor
{
public: 
  rp_ftor(Renderer *r, RayList *rl) : raylist(rl), renderer(r) {}
  ~rp_ftor() {}
  
  virtual void operator()()
  {
#if 0
		int chip, core;
		tacc_rdtscp(&chip, &core);
		std::cerr << chip << "::" << core << "\n";
#endif
		renderer->ProcessRays(raylist);
  }

private:
  RayList *raylist;
  Renderer *renderer;
};

void *
RayQManager::theRayQWorker(void *d)
{
	RayQManager *theRayQManager = (RayQManager *)d;

	register_thread("theRayQWorker");
	
	RayList *r;
	while ((r = theRayQManager->Dequeue()) != NULL)
	{
#if 1
    ThreadPool *threadpool = GetTheApplication()->GetTheThreadPool();
    threadpool->postWork<void>(rp_ftor(theRayQManager->GetTheRenderer(), r));
#else
		theRayQManager->GetTheRenderer()->ProcessRays(r);
#endif

	}

	theRayQManager->Unlock();
	pthread_exit(0);
}

RayQManager::RayQManager(Renderer *r)
{
	RayQManager::theRayQManager = this;
  renderer = r;

	pthread_mutex_init(&lock, NULL);
	pthread_cond_init(&wait, NULL);

	paused = false;
  done = false;

	pthread_create(&tid, NULL, RayQManager::theRayQWorker, this);
}

void
RayQManager::Kill()
{
	Lock();
	done = true;
	Signal();
	Unlock();
}

RayQManager::~RayQManager()
{
  Kill();
	pthread_mutex_destroy(&lock);
	pthread_cond_destroy(&wait);
	pthread_join(tid, NULL);
}

void
RayQManager::Lock()
{
  pthread_mutex_lock(&lock);
}

void
RayQManager::Unlock()
{
  pthread_mutex_unlock(&lock);
}

void
RayQManager::Wait()
{
  pthread_cond_wait(&wait, &lock);
}

void
RayQManager::Signal()
{
  pthread_cond_signal(&wait);
}

RayList *
RayQManager::Dequeue()
{
	RayList *r = NULL;

	Lock();

#if defined(EVENT_TRACKING)

	if (!done && rayQ.empty())
	{
		double t0 = EventTracker::gettime();

		while (!paused && !done && rayQ.empty())
			Wait();

		double t1 = EventTracker::gettime();

		class WaitForRaysEvent : public Event
		{
			public:
				WaitForRaysEvent(double t) : wait(t) {}

			protected:
				void print(ostream& o)
				{
					Event::print(o);
					o << "waited " << wait << " for ray list or done signal";
				}

		private:
			double wait;
		};

		GetTheEventTracker()->Add(new WaitForRaysEvent(t1 - t0));
	}

#else

	while (!done && (paused || rayQ.empty()))
		Wait();

#endif

	if (! rayQ.empty())
	{
		r = rayQ.front();
		rayQ.pop_front();
	}

  if (!r && !done)
     cerr << "R IS NULL, done is NOT DONE rayQ.empty is " << rayQ.empty() << " size is " << rayQ.size() << "\n";

	Unlock();

	return r;
}

#ifdef PVOL_SYNCHRONOUS
void
RayQManager::Pause()
{
  if (paused)
	{
		std::cerr << "RayQManager::Pause called while paused\n";
		exit(0);
	}

	Lock();
	paused = true;
	Unlock();
}

void
RayQManager::Resume()
{
	Lock();
  if (! paused)
	{
		std::cerr << "RayQManager::Resume called while not paused\n";
		exit(0);
	}
	paused = false;
	Signal();
	Unlock();
}
#endif

void
RayQManager::Enqueue(RayList *r)
{
	// This is called when a ray list is introduced - either
	// the initial local rays or from another process.

  if (! r)
    std::cerr << "Enqueuing NULL raylist!\n";

  Lock();

	rayQ.push_back(r);

	if (! paused)
		Signal();

  Unlock();
}

