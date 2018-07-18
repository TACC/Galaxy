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

#include <vector>

#include "dtypes.h"
#include "KeyedObject.h"
#include "Lighting.h"
#include "pthread.h"
#include "Rays.h"
#include "Rendering.h"
#include "RenderingEvents.h"
#include "RenderingSet.h"
#include "TraceRays.h"

namespace gxy
{

class Camera;
class RayQManager;
class Pixel;
class RayList;

KEYED_OBJECT_POINTER(Renderer)

class Renderer : public KeyedObject
{
  KEYED_OBJECT(Renderer)
    
public:
  static void Initialize();

  virtual ~Renderer();
  virtual void initialize();

  void ProcessRays(RayList *);
  void SendRays(RayList *, int);

  void SetEpsilon(float e);

  static Renderer *theRenderer;
  static Renderer *GetTheRenderer() { return theRenderer; }
  
  RayQManager *GetTheRayQManager() { return rayQmanager; }
  
  Lighting *GetTheLighting() { return &lighting; }

  virtual void LoadStateFromDocument(rapidjson::Document&);
  virtual void SaveStateToDocument(rapidjson::Document&);

  virtual void localRendering(RenderingSetP, MPI_Comm c);

  void LaunchInitialRays(RenderingSetP, RenderingP, std::vector<std::future<void>>&);

  virtual int SerialSize();
  virtual unsigned char *Serialize(unsigned char *);
  virtual unsigned char *Deserialize(unsigned char *);

	virtual void Render(RenderingSetP);
	int GetFrame() { return frame; }

	void DumpStatistics();
	void _dumpStats();

	void _sent_to(int d, int n)
	{
		pthread_mutex_lock(&lock);
		sent_to[d] += n;
		pthread_mutex_unlock(&lock);
	}

	void _received_from(int d, int n)
	{
		pthread_mutex_lock(&lock);
		received_from[d] += n;
		pthread_mutex_unlock(&lock);
	}

	void add_originated_ray_count(int n)
	{
		pthread_mutex_lock(&lock);
		originated_ray_count += n;
		pthread_mutex_unlock(&lock);
	}

	Lighting *get_the_lights() { return &lighting; }

	void Trace(RayList *);

	int GetMaxRayListSize() { return max_rays_per_packet; }

private:

	std::vector<std::future<void>> rvec;

	int frame;

	int max_rays_per_packet;

	int sent_ray_count;
	int terminated_ray_count;
	int originated_ray_count;
	int secondary_ray_count;
	int sent_pixels_count;
	int ProcessRays_input_count;
	int ProcessRays_continued_count;
	int *sent_to;
	int *received_from;

  TraceRays tracer;
  Lighting  lighting;
  RayQManager *rayQmanager;

  pthread_mutex_t lock;
  pthread_cond_t cond; 
  
  class StatisticsMsg : public Work
  {
  public:
    StatisticsMsg(Renderer *r) : StatisticsMsg(sizeof(Key))
		{
			unsigned char *p = (unsigned char *)contents->get();
			*(Key *)p = r->getkey();
		}

    WORK_CLASS(StatisticsMsg, true);

  public:
		bool CollectiveAction(MPI_Comm coll_comm, bool isRoot);
  };

public:
  class SendRaysMsg : public Work
  {
  public:
    SendRaysMsg(RayList *rl) : SendRaysMsg(rl->get_ptr()) {};
    WORK_CLASS(SendRaysMsg, false);

  public:
    bool Action(int sender);
  };

#ifdef GXY_SYNCHRONOUS

  class AckRaysMsg : public Work
  {
  public:
    AckRaysMsg(RenderingSetP rs);
    
    WORK_CLASS(AckRaysMsg, false);

  public:
    bool Action(int sender);
  };

#endif // GXY_SYNCHRONOUS

  class SendPixelsMsg : public Work
  {
  private:
    struct pix 
    {
      int x, y;
      float r, g, b, o;
    };

		struct hdr
		{
			Key rkey;
			Key rskey;
			int count;
			int frame;
			int source;
		};

		RenderingSetP rset;
    
  public:
    SendPixelsMsg(RenderingP r, RenderingSetP rs, int frame, int n) : SendPixelsMsg(sizeof(hdr) + (n * sizeof(Pixel)))
    {
			rset = rs;

      hdr *h    = (hdr *)contents->get();
			h->rkey   = r->getkey();
			h->rskey  = rs->getkey();
			h->frame  = frame;
			h->count  = n;
			h->source = GetTheApplication()->GetRank();

      nxt = 0;
    }

		void Send(int i)
		{
      hdr *h    = (hdr *)contents->get();

#ifdef GXY_EVENT_TRACKING
			GetTheEventTracker()->Add(new SendPixelsEvent(h->count, h->rkey, h->frame, i));
#endif

			Work::Send(i);

#ifdef GXY_SYNCHRONOUS
      rset->SentPixels(h->count);
#endif
		}

    void StashPixel(RayList *rl, int i) 
    {
      Pixel *p = (Pixel *)((((unsigned char *)contents->get()) + sizeof(hdr))) + nxt++;

      p->x = rl->get_x(i);
      p->y = rl->get_y(i);
      p->r = rl->get_r(i);
      p->g = rl->get_g(i);
      p->b = rl->get_b(i);
      p->o = rl->get_o(i);
    }

    WORK_CLASS(SendPixelsMsg, false);

  public:
    bool Action(int s)
    {
			hdr *h = (hdr *)contents->get();
      Pixel *pixels = (Pixel *)(((unsigned char *)contents->get()) + sizeof(hdr));

      RenderingP r = Rendering::GetByKey(h->rkey);
      if (! r)
        return false;

      RenderingSetP rs = RenderingSet::GetByKey(h->rskey);
      if (! rs)
        return false;

#ifdef GXY_SYNCHRONOUS
      rs->ReceivedPixels(h->count);
#endif

#ifdef GXY_EVENT_TRACKING
			GetTheEventTracker()->Add(new RcvPixelsEvent(h->count, h->rkey, h->frame, s));
#endif

			if (h->frame == rs->GetCurrentFrame())
				r->AddLocalPixels(pixels, h->count, h->frame, h->source);

      return false;
    }

  private:
    int nxt;
  };

  class RenderMsg : public Work
  {
  public:
    RenderMsg(Renderer *, RenderingSetP);
    ~RenderMsg();

    WORK_CLASS(RenderMsg, true);

    bool CollectiveAction(MPI_Comm, bool isRoot);

	private:
		int frame;
  };
};

} // namespace gxy
