#pragma once 

#include <vector>

#include "dtypes.h"
#include "pthread.h"
#include "KeyedObject.h"

#include "Rays.h"
#include "Rendering.h"
#include "RenderingSet.h"
#include "Lighting.h"
#include "TraceRays.h"
#include "RenderingEvents.h"

class Camera;
class RayQManager;
class Pixel;
class RayList;

using namespace std;

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
  
  virtual void LoadStateFromDocument(Document&);
  virtual void SaveStateToDocument(Document&);

  virtual void localRendering(RenderingSetP, MPI_Comm c);

  void LaunchInitialRays(RenderingSetP, RenderingP, vector<future<void>>&);

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


private:
	int frame;

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

  class SendRaysMsg : public Work
  {
  public:
    SendRaysMsg(RayList *rl) : SendRaysMsg(rl->get_ptr()) {};
    WORK_CLASS(SendRaysMsg, false);

  public:
    bool Action(int sender);
  };

#ifdef PVOL_SYNCHRONOUS

  class AckRaysMsg : public Work
  {
  public:
    AckRaysMsg(RenderingSetP rs);
    
    WORK_CLASS(AckRaysMsg, false);

  public:
    bool Action(int sender);
  };

#endif // PVOL_SYNCHRONOUS

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

#if defined(EVENT_TRACKING)
			GetTheEventTracker()->Add(new SendPixelsEvent(h->count, h->rkey, h->frame, i));
#endif

			Work::Send(i);

#ifdef PVOL_SYNCHRONOUS
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

#ifdef PVOL_SYNCHRONOUS
      rs->ReceivedPixels(h->count);
#endif

#if defined(EVENT_TRACKING)
			GetTheEventTracker()->Add(new RcvPixelsEvent(h->count, h->rkey, h->frame, s));
#endif

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
