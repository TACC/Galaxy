// ========================================================================== //
// Copyright (c) 2014-2019 The University of Texas at Austin.                 //
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

/*! \file RayQManager.h 
 * \brief the manager for RayList processing at each node
 * \ingroup render
 */

#include <list>
#include <pthread.h>
#include <time.h>

#include "Application.h"
#include "Rays.h"
#include "Renderer.h"
#include "Work.h"

namespace gxy
{

class Renderer;

//! the manager for RayList processing at each node
/*! \ingroup render */
class RayQManager
{
public:
	RayQManager(Renderer *); //!< constructor
	~RayQManager(); //!< default destructor

	//! returns a pointer to the RayList manager singleton for this process
	static RayQManager *GetTheRayQManager();
	static RayQManager 	*theRayQManager; //!< the RayList manager singleton

	// These are used to protect enqueuing and dequeuing.
	void Lock(); //!< lock the queue (e.g. to perform an atomic action on it)
	void Unlock(); //!< unlock the queue
	void Wait(); //!< wait until a Signal is received
	void Signal(); //!< resume processing after a Wait

	void Kill(); //!< terminate processing for this ray queue

	void Enqueue(RayList *r); //!< add the given RayList to this ray queue
	RayList *Dequeue(); //!< remove a RayList from this ray queue

	//! global check across all processes whether the rendering is done
	/*! this call is used when Galaxy writes images to determine if a frame is done and the image can be written
	 */
	void CheckWhetherDone()
	{
		SynchronousCheckMsg *msg = new SynchronousCheckMsg(0);
		msg->Broadcast(true, true);
	}

	// The following is used to manage the overall rendering state... Renderer::Render,
	// called from IceT's callback function, can't return until all rendering is
	// done - that is, when all rays are retired throughout the system - because
	// when it does, IceT will start compositing.  So Render waits on the transition
	// from Busy to Idle.  This is signaled from the message loop when the master determines
	// that its done.

	int current_frame_id() { return frame_id; } //!< return the current frame being rendered
	void bump_frame_id() { frame_id ++; } //!< move to the next frame to render

#ifdef GXY_WRITE_IMAGES
	void Pause(); //!< pause queue processing, used when writing an image
	void Resume(); //!< resume queue processing after a Pause
	//! put the number of RayList objects and the total number of Rays into the given ints
	/*! \param n receives the number of RayList objects
	 * \param k receives the total number of Rays
	 */
	void  GetQueuedRayCount(int& n, int& k); 
#endif
    
private:
    Renderer *renderer;

	pthread_mutex_t lock;
	pthread_cond_t  wait;
	pthread_t       tid;
	
	bool paused;
	int frame_id;

	// The following is the number of ray lists active in 
	// this process - the  number in the RayQ and one for
	// a ray list being processed (if there is one).  So
	// we increment it when a ray list is added to the RayQ
	// and decrement it when we have completed the processing
	// of a ray list we pulled off the RayQ.

	// int local_raylist_count;	

	bool done;
	static void   			*theRayQWorker(void *d);

	class SendStateMsg : public Work
  {
  public:
    SendStateMsg(int p, bool busy, int frame_id) : SendStateMsg(sizeof(int) + sizeof(bool) + sizeof(int))
    {
			unsigned char *ptr = contents->get();
			*(int *)ptr = p;
			ptr += sizeof(int);
			*(bool *)ptr = busy;
			ptr += sizeof(bool);
			*(int *)ptr = frame_id;
    }

    WORK_CLASS(SendStateMsg, false);

  public:
    bool Action(int sender);
  };

	class SynchronousCheckMsg : public Work
  {
  public:
    WORK_CLASS(SynchronousCheckMsg, true);

  public:
    bool CollectiveAction(MPI_Comm c, bool isRoot);
  };

	std::list<RayList*> rayQ;
};

} // namespace gxy
