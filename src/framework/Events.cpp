#include "pvol.h"

#include <limits>
#include <time.h>
#include <sys/time.h>

#ifdef __MACH__
#include <mach/clock.h>
#include <mach/mach.h>
#endif

#include "Events.h"
#include "Application.h"

typedef std::numeric_limits< double > dbl;

EventTracker *theEventTracker;
EventTracker::EventTracker()
{
  theEventTracker = this; 
}

EventTracker *GetTheEventTracker() { return theEventTracker; }

pthread_mutex_t EventsLock = PTHREAD_MUTEX_INITIALIZER;

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
}

Event::~Event()
{
  std::cerr << "ev dtor\n";
}

void
Event::print(ostream& o)
{
	o << fixed << time << ": ";
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
	pthread_mutex_lock(&EventsLock);
#if 1
	// events.push_back(shared_ptr<Event>(e));
	events.push_back(e);
#else 
	int rank = GetTheApplication()->GetRank();
	std::fstream fs;
	std::stringstream fname;
	f {}name << "events_" << rank;
	fs.open(fname.str().c_str(), std::fstream::out | std::ofstream::app);
	e->Print(fs);
	fs << "\n";
	fs.close();
	delete e;
#endif
	pthread_mutex_unlock(&EventsLock);
}

