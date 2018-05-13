#pragma once

#include <iostream>
#include <fstream>
#include <memory>
#include <pthread.h>
#include <vector>

#include "KeyedObject.h"

namespace pvol
{
extern pthread_mutex_t EventsLock;



using namespace pvol;

class Event
{
public:
	Event();
	~Event();

	void Print(ostream& o) { print(o); }

protected:
	virtual void print(ostream&);

private:
	double time;
};

class EventTracker
{
public:
	EventTracker();

	void DumpEvents();
	void DumpEvents(fstream& fs);

	void Add(Event *e);

	static double gettime();

	bool is_empty() { return events.size() == 0; }
	
private:
	int event_dump_count;
	vector<Event*> events;
};

extern EventTracker *GetTheEventTracker();
}

