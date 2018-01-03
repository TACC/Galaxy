#pragma once

#include <iostream>
#include <fstream>
#include <memory>
#include <pthread.h>

#include "KeyedObject.h"

extern pthread_mutex_t EventsLock;

#include <vector>

using namespace std;

class Event
{
public:
	Event();
	~Event(){}

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

	void Add(Event *e);

	void DumpEvents(ostream& o);

	static double gettime();

	bool is_empty() { return events.size() == 0; }
	
private:
	vector<shared_ptr<Event>> events;
};

extern EventTracker *GetTheEventTracker();
