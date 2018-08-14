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
#include "Threading.h"

using namespace std;

namespace gxy
{

typedef numeric_limits< double > dbl;

EventTracker *GetTheEventTracker() { return GetTheApplication()->GetTheThreadManager()->get_events(); }

pthread_mutex_t EventsLock = PTHREAD_MUTEX_INITIALIZER;

Event::Event()
{
	time = EventTracker::gettime();
}

Event::~Event()
{
  cerr << "event dtor" << endl;
}

void
Event::print(ostream& o)
{
	o << fixed << time << ": ";
}

EventTracker::EventTracker()
{
}

EventTracker::~EventTracker()
{
}


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

void
EventTracker::DumpEvents()
{
#if defined(GXY_EVENT_TRACKING)
	int rank = GetTheApplication()->GetTheMessageManager()->GetRank();
	int tid = GetTheApplication()->GetTheThreadManager()->get_index();
	fstream fs;
	stringstream fname;
	fname << "gxy_events_" << "_" << rank << "_" << tid;
	fs.open(fname.str().c_str(), fstream::out);
	DumpEvents(fs);
	fs.close();
#endif
}

void
EventTracker::DumpEvents(fstream& fs)
{
	for (auto e : events)
	{
		e->Print(fs);
		fs << "\n";
	}
}

void
EventTracker::Add(Event *e)
{
	events.push_back(e);
}

} // namespace gxy
