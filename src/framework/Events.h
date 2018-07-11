#pragma once

#include <iostream>
#include <fstream>
#include <memory>
#include <pthread.h>
#include <vector>

#include "KeyedObject.h"

extern pthread_mutex_t EventsLock;


namespace gxy
{
class Event
{
public:
	Event();
	~Event(){}

	void Print(std::ostream& o) { print(o); }

protected:
	virtual void print(std::ostream&);

private:
	double time;
};

class EventTracker
{
public:
	EventTracker();

	void Add(Event *e);

	void DumpEvents(std::ostream& o);

	static double gettime();

	bool is_empty() { return events.size() == 0; }
	
private:
	std::vector<std::shared_ptr<Event>> events;
};

extern EventTracker *GetTheEventTracker();
}