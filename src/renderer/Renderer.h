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

  void LaunchInitialRays(RenderingP, vector<future<void>>&);

  virtual int SerialSize();
  virtual unsigned char *Serialize(unsigned char *);
  virtual unsigned char *Deserialize(unsigned char *);

	virtual void Render(RenderingSetP);
	int GetFrame() { return frame; }

private:
	int frame;

  TraceRays tracer;
  Lighting  lighting;
  RayQManager *rayQmanager;

  pthread_mutex_t lock;
  pthread_cond_t cond; 
  
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
    
  public:
    SendPixelsMsg(RenderingP r, int n) : SendPixelsMsg(sizeof(Key) + 2*sizeof(int) + (n * sizeof(Pixel)))
    {
      unsigned char *p = (unsigned char *)contents->get();
      *(Key *)p = r->getkey();
      p += sizeof(Key);
			*(int *)p = r->GetFrame();
			if (r->GetFrame() < 0)
				std::cerr << "SendPixelsMsg FRAME ERROR!\n";
			p += sizeof(int);
      *(int *)p = n;

      nxt = 0;
    }

    void StashPixel(RayList *rl, int i) 
    {
      Pixel *p = (Pixel *)((((unsigned char *)contents->get()) + 2*sizeof(int) + sizeof(Key))) + nxt++;

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
      unsigned char *p = (unsigned char *)contents->get();
      Key k = *(Key *)p;
      p += sizeof(Key);
			int frame = *(int *)p;
			p += sizeof(int);
      int n  = *(int *)p;
      p += sizeof(int);
      Pixel *pixels = (Pixel *)p;

      RenderingP ren = Rendering::GetByKey(k);
      if (! ren)
      {
        APP_PRINT(<< "SendPixelsMsg (" << k << ") DROPPED");
        return false;
      }

#ifdef PVOL_SYNCHRONOUS
      ren->GetTheRenderingSet()->ReceivedPixels(n);

#endif // PVOL_SYNCHRONOUS

			// std:cerr << "rendering frame: " << ren->GetFrame() << " frame from message " << frame << "\n";
			ren->AddLocalPixels(pixels, n, frame);

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

  public:
    bool CollectiveAction(MPI_Comm, bool isRoot);
  };
};
