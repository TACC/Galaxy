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

  virtual void localRendering(RenderingSetP, MPI_Comm);

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
			Key key;
			int count;
			int frame;
			int source;
		};
    
  public:
    SendPixelsMsg(RenderingP r, int frame, int n) : SendPixelsMsg(sizeof(hdr) + (n * sizeof(Pixel)))
    {
      hdr *h = (hdr *)contents->get();
			h->key = r->getkey();
			h->frame = frame;
			h->count = n;
			h->source = GetTheApplication()->GetRank();

      nxt = 0;
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

			extern int debug_target;
			if (h->source == debug_target)
				std::cerr << "SendPix action from debug_target\n";

      RenderingP ren = Rendering::GetByKey(h->key);
      if (! ren)
      {
        // APP_PRINT(<< "SendPixelsMsg (" << frame << ") DROPPED");
        return false;
      }

#ifdef PVOL_SYNCHRONOUS
      ren->GetTheRenderingSet()->ReceivedPixels(h->count);

#endif // PVOL_SYNCHRONOUS

			// std:cerr << "rendering frame: " << ren->GetFrame() << " frame from message " << frame << "\n";
			ren->AddLocalPixels(pixels, h->count, h->frame, h->source);

#if defined(EVENT_TRACKING)

      class ReceivedPixelContributionsEvent : public Event
      {
      public:
        ReceivedPixelContributionsEvent(int c, int s, RenderingP r) : count(c), src(s), rset(r->GetTheRenderingSetKey()) {}

      protected:
        void print(ostream& o)
        {
          Event::print(o);
          o << "received " << count << " pixels from " << src << " for rset " << rset;
        }

      private:
        int count;
        int src;
        Key rset;
      };

      GetTheEventTracker()->Add(new ReceivedPixelContributionsEvent(n, s, ren));
#endif

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
