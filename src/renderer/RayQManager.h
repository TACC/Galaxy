#pragma once

#include <pthread.h>
#include <list>
#include <time.h>

#include "Application.h"
#include "Rays.h"
#include "Work.h"
#include "Renderer.h"

#include "debug.h"

using namespace std;

class Renderer;

class RayQManager
{
public:
	RayQManager(Renderer *);
	~RayQManager();

	static RayQManager *GetTheRayQManager();
	static RayQManager 	*theRayQManager;

	void ResetState();

	// These are used to protect enqueuing and dequeuing.

	void Lock();
	void Unlock();
	void Wait();
	void Signal();

	void Kill();

	void Enqueue(RayList *r);
	RayList *Dequeue();

	void CheckState();
	void UpdateChildState(bool busy, int child);

	void CheckWhetherDone()
	{
		SynchronousCheckMsg *msg = new SynchronousCheckMsg(0);
		msg->Broadcast(true, true);
	}

	// The following is used to manage the overall rendering state... Renderer::Render,
	// called from IceT's callback function, can't return until all rendering is
	// done - that is, when all rays are retired throughout the system - because
	// when it does, IceT will start compositing.  So Render waits on the transition
	// from Busy to Idle.  This is signalled from the message loop when the master determines
	// that its done.

	int current_frame_id() { return frame_id; }
	void bump_frame_id() { frame_id ++; }

#ifdef PVOL_SYNCHRONOUS
	void Pause();
	void Resume();
	void  GetQueuedRayCount(int&, int&);
#endif
    
    Renderer *GetTheRenderer() { return renderer; }

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

	list<RayList*> rayQ;
};
