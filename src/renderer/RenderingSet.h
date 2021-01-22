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

/*! \file RenderingSet.h 
 * \brief represents a set of Rendering objects
 * \ingroup render
 */

#include <memory>
#include <string>
#include <vector>
#include <map>

#include "GalaxyObject.h"
#include "Datasets.h"
#include "Rays.h"
#include "Work.h"

namespace gxy
{

KEYED_OBJECT_POINTER_TYPES(RenderingSet)

class Camera;
class RayList;

// Manage completeness of a SET of renderings
//! represents a set of Rendering objects
/*! a set of images of a Visualization rendered using a certain Camera 
 * \sa KeyedObject, Rendering, Visualization, Camera
 * \ingroup render
 */
class RenderingSet : public KeyedObject, public std::enable_shared_from_this<RenderingSet>
{
  friend class Rendering;
  friend class PropagateStateMsg;
  friend class SynchronousCheckMsg;
  friend class SaveImagesMsg;
  friend class ResetMsg;
  
  KEYED_OBJECT(RenderingSet);

public:
  virtual void initialize(); //!< initialize this RenderingSet
  virtual ~RenderingSet(); //!< default destructor

  void AddRendering(RenderingDPtr r); //!< add a Rendering to this RenderingSet
  int  GetNumberOfRenderings(); //!< return the number of Renderings in this RenderingSet
  //! return a pointer to the Rendering at the requested index
  /*! \warning currently not range-checked, behavior of an out-of-range request is not defined
   */
  RenderingDPtr GetRendering(int i);

  //! save images for this RenderingSet using the supplied base name for the image files.  If asFLoat is true, then save four-channel fload images in fits format; otherwise, write a png color image.
  void SaveImages(std::string basename, bool asFloat=false); 

  //! Add the given RayList to be processed against this RenderingSet
  /*! Add a raylist to the queue of raylists to be processed
   * Go through the RenderingSet so we can manage the number
   * of raylists currently alive separately for each RenderingSet
   * If silent, then the modification of the state does not cause
   * a notification upstream; this is used during the initial
   * spawning of the rays.
   */
  void Enqueue(RayList *, bool silent=false);

  virtual int serialSize(); //!< return the size in bytes of this RenderingSet
  //! serialize this RenderingSet to the given byte stream
  virtual unsigned char* serialize(unsigned char *ptr); 
  //! deserialize a RenderingSet from the given byte stream into this object
  virtual unsigned char* deserialize(unsigned char *ptr);

#ifdef GXY_WRITE_IMAGES

  int activeCameraCount; //!< number of cameras in this RenderingSet

  //! increase the number of active cameras by one
  void IncrementActiveCameraCount(); 
  //! decrease the number of active cameras by one and add the rays spawned by the camera to the overall ray count
  /*! /param rayknt the number of spawned rays to add to the count
   */
  void DecrementActiveCameraCount(int rayknt); 
  //! return whether there is at least one active camera in this RenderingSet
  bool CameraIsActive() { return activeCameraCount > 0; }

  void IncrementInFlightCount(); //!< increase the inflight count by one
  void DecrementInFlightCount(); //!< decrease the inflight count by one

  //! return whether rendering for this RenderingSet is complete
  bool IsDone() { return done; }

  void Reset();  //!< broadcast a ResetMsg to all Galaxy processes
  void WaitForDone(); //!< stop rendering at this process and wait for a FinishedRendering event
  bool Busy(); //!< returns whether this process is actively rendering

  void local_reset(); //!< reset local state in response to a ResetMsg
  bool first_async_completion_test_done;

  void InitializeState(); //!< initialize local state for this RenderingSet

  //! reduce the count of active RayLists for this RenderingSet by one
  /*! Decrement the number of ray lists for this set that are alive
   * in this process.   If it had been 1, then state has changed.
   * This is called both when ProcessRays finishes a RayList and 
   * from the acknowlegement of the receipt of a  ray list
   * from the action of the RaycastRenderer's AckRays message,
   * so it needs to be accessible.
   */
  void DecrementRayListCount();

  //! increase the count of active RayLists for this RenderingSet by one
  /*! Increment the number of ray lists for this set that are alive
   * in this process.   If it had been 0, then state has changed.
   * If silent, then the modification of the state does not cause
   * a notification upstream; this is used during the initial
   * spawning of the rays.  This is called both when a ray list
   * is added to the local RayQ and when one is sent to a remote
   * process' RayQ so it needs to be accessible.
   */
  void IncrementRayListCount(bool silent = false);
  //! set initial state to begin rendering this RenderingSet
  void SetInitialState(int local_ray_count, int left_state, int right_state);
  //! return parent, left and right ids in the process tree
  void get_tree_info(int& p, int& l, int& r) { p = parent; l = left_id; r = right_id; }

  //! increase the sent pixels counter by a given amount
  void SentPixels(int k)
  {
    Lock();
    n_pix_sent += k;
    Unlock();
  }

  //! increase the received pixels counter by a given amount
  void ReceivedPixels(int k)
  {
    Lock();
    n_pix_received += k;
    Unlock();
  }

#ifdef GXY_PRODUCE_STATUS_MESSAGES
  void DumpState(); //!< broadcast a DumpStateMsg to all Galaxy processes
  void _dumpState(MPI_Comm, const char *); //!< dump local state to file in response to a DumpStateMsg
  void _initStateTimer(); //!< starts state timer
  double state_timer_start;
#endif

  //! resets count of spawned rays to zero
  void initializeSpawnedRayCount() { spawnedRayCount = 0; }
  //! returns the current count of rays spawned by this RenderingSet
  int getSpawnedRayCount() { return spawnedRayCount; }

  // int state_counter;

  //! finalize this RenderingSet after render is complete
  void Finalize()
  {
    SetDone();
    InitializeState();
    Signal();
  };
      
#endif // GXY_WRITE_IMAGES

  // set up ispc-level data structures 
  virtual bool local_commit(MPI_Comm);

protected:

  std::vector<RenderingDPtr> renderings;

#ifdef GXY_WRITE_IMAGES

  void Lock()   { pthread_mutex_lock(&lck);     }
  void Unlock() { pthread_mutex_unlock(&lck);   }
  void Signal() { pthread_cond_signal(&w8);     }
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

#endif // GXY_WRITE_IMAGES

public:

  //! return the current frame being rendered in this RenderingSet
  int  GetCurrentFrame() { return current_frame; }

  //! set the the next frame to render
  void SetRenderFrame(int n) { next_frame = n; }

  //! returns the frame number of the next frame to render, or -1 if no next frame exists
  /*! Called from Renderer::localRendering.  We won't generate initial rays
   * for is call to localRendering IF we've already seen a ray list from a 
   * subsequent frame and NeedInitialRays will return -1.   Otherwise, 
   * NeedInitialRays returns the new frame number;
   */
  int  NeedInitialRays();

  //! returns whether the given frame number is an active frame for this RenderingSet
  /*! Returns true of false depending on whether fnum is an active frame.
   * Just what an active frame is depends.   If we are in asyncronous mode,
   * we might consider only the latest frame active - in which case, if 
   * fnum > current_frame, then current_frame gets bumped and we return 
   * fnum == current_frame.   If we don't want ALL frames to be considered
   * active we just return true.
   */
  bool IsActive(int fnum);

  //! Set the datasets this rendering set will refer to
  void SetTheDatasets(DatasetsDPtr d) { datasets = d; }
  
  //! Get the datasets this rendering set will refer to
  DatasetsDPtr getTheDatasets() { return datasets; }

private:

  DatasetsDPtr datasets;

  int current_frame;
  int next_frame;
  int spawnedRayCount;

  class SaveImagesMsg : public Work
  {
  public:
    SaveImagesMsg(RenderingSet *r, std::string basename, bool isFloat);

    WORK_CLASS(SaveImagesMsg, true);

  public:
    bool CollectiveAction(MPI_Comm c, bool);
  };

#ifdef GXY_WRITE_IMAGES

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


#endif // GXY_WRITE_IMAGES
};

} // namespace gxy
