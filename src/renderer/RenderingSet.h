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

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "KeyedObject.h"
#include "Rays.h"
#include "Work.h"

namespace gxy
{

KEYED_OBJECT_POINTER(RenderingSet)

class Camera;
class RayList;

// Manage completeness of a SET of renderings

class RenderingSet : public KeyedObject, public std::enable_shared_from_this<RenderingSet>
{
	friend class Rendering;
	friend class PropagateStateMsg;
	friend class SynchronousCheckMsg;
	friend class SaveImagesMsg;
	friend class ResetMsg;
	
	KEYED_OBJECT(RenderingSet);

public:
	virtual void initialize();
	virtual ~RenderingSet();

	void AddRendering(RenderingP r);
	int  GetNumberOfRenderings();
	RenderingP GetRendering(int i);

	void SaveImages(std::string basename);

	// Add a raylist to the queue of raylists to be processed
	// Go through the RenderingSet so we can manage the number
	// of raylists currently alive separately for each RenderingSet
	// If silent, then the modification of the state does not cause
	// a notification upstream; this is used during the initial
	// spawning of the rays.

	void Enqueue(RayList *, bool silent=false);

  virtual int serialSize();
  virtual unsigned char* serialize(unsigned char *ptr);
  virtual unsigned char* deserialize(unsigned char *ptr);

#ifdef GXY_SYNCHRONOUS

	int activeCameraCount;

	void IncrementActiveCameraCount();
	void DecrementActiveCameraCount(int);
	bool CameraIsActive() { return activeCameraCount > 0; }

	void IncrementInFlightCount();
	void DecrementInFlightCount();

	bool IsDone() { return done; }

	void Reset();	
	void WaitForDone();
	bool Busy();

	void local_reset();
	bool first_async_completion_test_done;

	void InitializeState();

	// Decrement the number of ray lists for this set that are alive
	// in this process.   If it had been 1, then state has changed.
	// This is called both when ProcessRays finishes a RayList and 
	// from the acknowlegement of the receipt of a  ray list
	// from the action of the RaycastRenderer's AckRays message,
	// so it needs to be accessible.
	
	void DecrementRayListCount();

	// Increment the number of ray lists for this set that are alive
	// in this process.   If it had been 0, then state has changed.
	// If silent, then the modification of the state does not cause
	// a notification upstream; this is used during the initial
	// spawning of the rays.  This is called both when a ray list
	// is added to the local RayQ and when one is sent to a remote
	// process' RayQ so it needs to be accessible.
	
	void IncrementRayListCount(bool silent = false);
	void SetInitialState(int local_ray_count, int left_state, int right_state);
  void get_tree_info(int& p, int& l, int& r) { p = parent; l = left_id; r = right_id; }

	void SentPixels(int k)
	{
		Lock();
		n_pix_sent += k;
	  Unlock();
	}

	void ReceivedPixels(int k)
	{
		Lock();
		n_pix_received += k;
		Unlock();
	}

#ifdef GXY_PRODUCE_STATUS_MESSAGES
	void DumpState();
	void _dumpState(MPI_Comm, const char *);
	void _initStateTimer();
	double state_timer_start;
#endif

	void initializeSpawnedRayCount() { spawnedRayCount = 0; }
	int getSpawnedRayCount() { return spawnedRayCount; }

	// int state_counter;

	void Finalize()
	{
		SetDone();
		InitializeState();
		Signal();
	};
			
#endif // GXY_SYNCHRONOUS

protected:

	std::vector<RenderingP> renderings;

#ifdef GXY_SYNCHRONOUS

	void Lock()   { pthread_mutex_lock(&lck); 		}
	void Unlock() { pthread_mutex_unlock(&lck); 	}
	void Signal() { pthread_cond_signal(&w8); 		}
	void Wait()   { pthread_cond_wait(&w8, &lck); }

	void SetDone() { done = true; }

	// Am I possibly idle based on current child state and the number of 
	// pertinent queued ray lists

	void CheckLocalState();

	// Called when a message is received from child indicating that its 
	// state has changed.  Checks to see if local state has changed.  
	// If this is the root and local state has changed to idle, then we
	// might be done - do synchronous check.   Otherwise if the state has
	// changed, notify the parent

	void UpdateChildState(bool b, int c);

	// Called by the root when its 'local state' is set to idle, indicating
	// that the whole ball of wax might be done.   Calls the synchronous test

	void CheckGlobalState();
	
	int get_local_raylist_count() { return local_raylist_count; }
	void get_local_raylist_count(int &k) { k = local_raylist_count; }

	int get_local_inflight_count() { return local_inflight_count; }
	void get_local_inflight_count(int &k) { k = local_inflight_count; }

	int get_number_of_pixels_sent() { return n_pix_sent; }
	void get_number_of_pixels_sent(int &k) { k = n_pix_sent; }
	
	int get_number_of_pixels_received() { return n_pix_received; }
	void get_number_of_pixels_received(int &k) { k = n_pix_received; }

	// int get_state_counter() { return state_counter++; }

#endif // GXY_SYNCHRONOUS

public:

	int  GetCurrentFrame() { return current_frame; }

	// Called from Renderer::localRendering.  We won't generate initial rays
	// for is call to localRendering IF we've already seen a ray list from a 
	// subsequent frame and NeedInitialRays will return -1.   Otherwise, 
	// NeedInitialRays returns the new frame number;

	int  NeedInitialRays();
	bool KeepRays(RayList *rl);

private:

	int current_frame;
	int next_frame;
  int spawnedRayCount;

  class SaveImagesMsg : public Work
  {
  public:
		SaveImagesMsg(RenderingSet *r, std::string basename);

    WORK_CLASS(SaveImagesMsg, true);

  public:
    bool CollectiveAction(MPI_Comm c, bool);
  };

#ifdef GXY_SYNCHRONOUS

	bool done;

  bool currently_busy, last_busy, left_busy, right_busy;
  int left_id, right_id, parent;

	int local_inflight_count;
	int local_raylist_count;
	int n_pix_sent;

	int n_pix_received;	

	pthread_mutex_t local_lock;

	pthread_mutex_t lck;
	pthread_cond_t w8;

  class PropagateStateMsg : public Work
  {
  public:
    PropagateStateMsg(RenderingSet *rs, bool busy) 
				: PropagateStateMsg(sizeof(Key) + sizeof(bool))
    {
			unsigned char *ptr = (unsigned char *)contents->get();
      *(Key *)ptr = rs->getkey();
      ptr += sizeof(Key);
      *(bool *)ptr = busy;
    }

    WORK_CLASS(PropagateStateMsg, false);

  public:
    bool Action(int sender);
  };

  class SynchronousCheckMsg : public Work
  {
  public:
		SynchronousCheckMsg(Key k) : SynchronousCheckMsg(sizeof(Key))
		{
			*(Key *)contents->get() = k;
		}

    WORK_CLASS(SynchronousCheckMsg, true);

  public:
    bool CollectiveAction(MPI_Comm c, bool);
  };

  class ResetMsg : public Work
  {
  public:
		ResetMsg(RenderingSet *r);

    WORK_CLASS(ResetMsg, true);

  public:
    bool CollectiveAction(MPI_Comm c, bool);
  };

  class DumpStateMsg : public Work
  {
  public:
    DumpStateMsg(Key k) : DumpStateMsg(sizeof(Key))
		{
			*(Key *)contents->get() = k;
		}

    WORK_CLASS(DumpStateMsg, true);

  public:
    bool CollectiveAction(MPI_Comm c, bool);
  };


#endif // GXY_SYNCHRONOUS
};

} // namespace gxy
