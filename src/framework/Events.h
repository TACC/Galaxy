// ========================================================================== //
// Copyright (c) 2014-2020 The University of Texas at Austin.                 //
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

#pragma once

/*! \file Events.h 
 * \brief expresses and tracks events in the Galaxy system
 * \ingroup framework
 */

#include <fstream>
#include <iostream>
#include <memory>
#include <pthread.h>
#include <vector>

#include "GalaxyObject.h"

namespace gxy
{

//! marks a point in time
/*! \ingroup framework
 * \sa EventTracker
 */
class Event
{
public:
	Event(); //!< default constructor
	~Event(); //!< default destructor

	//! print this event timestamp
	void Print(std::ostream& o) { print(o); }

protected:
	virtual void print(std::ostream&);

private:
	double time;
};

//! manages timed events
/*! \ingroup framework
 * \sa Event
 */
class EventTracker
{
public:
	EventTracker(); //!< default constructor
	~EventTracker(); //!< default destructor

	//! dump events to file `gxy_events_R_T` where `R` is process rank and `T` is the thread index
	void DumpEvents();
	//! dump events to the given file stream
	void DumpEvents(std::fstream& fs);

	//! add an Event to this tracker
	void Add(Event *e);

	//! get an architecture-appropriate representation of the current time in seconds
	static double gettime();

	//! false if any events have been added by Add
	bool is_empty() { return events.size() == 0; }
	
private:
	int event_dump_count;
	std::vector<Event*> events;
};

/*! \ingroup framework
 * @{
 */

//! lock used to synchronize update to the EventTracker
extern pthread_mutex_t EventsLock;
//! return the EventTracker singleton
extern EventTracker *GetTheEventTracker();

/*! }@ */ // group framework

} // namespace gxy
