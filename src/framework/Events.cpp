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

#include "galaxy.h"

#include <limits>
#include <time.h>
#include <sys/time.h>

#ifdef __MACH__
#include <mach/clock.h>
#include <mach/mach.h>
#endif

#include "Events.h"
#include "Application.h"

using namespace std;

namespace gxy
{
typedef std::numeric_limits< double > dbl;

EventTracker *theEventTracker;
EventTracker::EventTracker()
{
  theEventTracker = this; 
}

EventTracker *GetTheEventTracker() { return theEventTracker; }

pthread_mutex_t EventsLock;

double
EventTracker::gettime()
{
#ifdef __MACH__ 
  clock_serv_t cclock;
  mach_timespec_t mts;
  host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
  clock_get_time(cclock, &mts);
  mach_port_deallocate(mach_task_self(), cclock);
  return mts.tv_sec + (mts.tv_nsec / 1000000000.0);
#else
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  return ts.tv_sec + (ts.tv_nsec / 1000000000.0);
#endif
}

Event::Event()
{
	time = EventTracker::gettime();
	// theEventTracker->Add(this); 
}

void
Event::print(ostream& o)
{
	o << fixed << time << ": ";
}

void
EventTracker::DumpEvents(ostream& o)
{
	cout.precision(dbl::max_digits10);
	for (auto e : events)
	{
		e->Print(o);
		o << "\n";
	}
}

void
EventTracker::Add(Event *e)
{
	pthread_mutex_lock(&EventsLock);
#if 0
	events.push_back(shared_ptr<Event>(e));
#else 
	int rank = GetTheApplication()->GetRank();
	std::fstream fs;
	std::stringstream fname;
	fname << "events_" << rank;
	fs.open(fname.str().c_str(), std::fstream::out | std::ofstream::app);
	e->Print(fs);
	fs << "\n";
	fs.close();
	delete e;
#endif
	pthread_mutex_unlock(&EventsLock);
}

}