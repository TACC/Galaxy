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

#include <iostream>
#include <sstream>
#include <pthread.h>

#include "Application.h"
#include "Threading.h"
#include "Renderer.h"
#include "RayQManager.h"
#include "Rays.h"

using namespace std;

namespace gxy 
{

#if defined(GXY_EVENT_TRACKING)

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

void *
RayQManager::theRayQWorker(void *d)
{
	RayQManager *theRayQManager = (RayQManager *)d;

	register_thread("theRayQWorker");
	
	RayList *r;
	while ((r = theRayQManager->Dequeue()) != NULL)
		r->GetTheRenderer()->ProcessRays(r);

	theRayQManager->Unlock();
	pthread_exit(0);
}

RayQManager::RayQManager(Renderer *r)
{
  renderer = r;

	pthread_mutex_init(&lock, NULL);
	pthread_cond_init(&wait, NULL);

	paused = false;
  done = false;

  GetTheApplication()->GetTheThreadManager()->create_thread(string("rayQWorker"), &tid, NULL, RayQManager::theRayQWorker, this);
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

#if defined(GXY_EVENT_TRACKING)

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
     cerr << "R IS NULL, done is NOT DONE rayQ.empty is " << rayQ.empty() << " size is " << rayQ.size() << endl;

	Unlock();

	return r;
}

#ifdef GXY_WRITE_IMAGES
void
RayQManager::Pause()
{
  if (paused)
	{
		cerr << "ERROR: RayQManager::Pause called while paused" << endl;
		exit(1);
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
		cerr << "ERROR: RayQManager::Resume called while not paused" << endl;
		exit(1);
	}
	paused = false;
	Signal();
	Unlock();
}

void
RayQManager::GetQueuedRayCount(int& n, int& k)
{
	Lock();

	n = 0, k = 0;
	for (auto a : rayQ)
	{
		n ++;
		k = k + a->GetRayCount();
	}

	Unlock();
}
#endif

void
RayQManager::Enqueue(RayList *r)
{
	// This is called when a ray list is introduced - either
	// the initial local rays or from another process.

	
  if (r->GetTheRenderingSet()->IsActive(r->GetFrame()))
  {
    if (! r)
			cerr << "WARNING: Enqueuing NULL raylist!" << endl;

		Lock();

		rayQ.push_back(r);

		if (! paused)
			Signal();

		Unlock();
	}
	else
    delete r;
}

} // namespace gxy
