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

namespace pvol
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

  virtual void LoadStateFromDocument(Document&);
  virtual void SaveStateToDocument(Document&);

  virtual void localRendering(RenderingSetP, MPI_Comm);

  void LaunchInitialRays(RenderingP, vector<future<void>>&);

  virtual int SerialSize();
  virtual unsigned char *Serialize(unsigned char *);
  virtual unsigned char *Deserialize(unsigned char *);

	virtual void Render(RenderingSetP);

private:
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

  class AckRaysMsg : public Work
  {
  public:
    AckRaysMsg(RenderingSetP rs);
    
    WORK_CLASS(AckRaysMsg, false);

  public:
    bool Action(int sender);
  };

  class SendPixelsMsg : public Work
  {
  private:
    struct pix 
    {
      int x, y;
      float r, g, b, o;
    };
    
  public:
    SendPixelsMsg(RenderingP r, int n) : SendPixelsMsg(sizeof(Key) + sizeof(int) + (n * sizeof(Pixel)))
    {
      unsigned char *p = (unsigned char *)contents->get();
      *(Key *)p = r->getkey();
      p += sizeof(Key);
      *(int *)p = n;

      nxt = 0;
    }

    void StashPixel(RayList *rl, int i) 
    {
      Pixel *p = (Pixel *)((((unsigned char *)contents->get()) + sizeof(int) + sizeof(Key))) + nxt++;

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
      int n  = *(int *)p;
      p += sizeof(int);
      Pixel *pixels = (Pixel *)p;

      RenderingP ren = Rendering::GetByKey(k);
      if (! ren)
      {
        APP_PRINT(<< "SendPixelsMsg (" << k << ") DROPPED");
        return false;
      }

      ren->GetTheRenderingSet()->ReceivedPixels(n);

      ren->AddLocalPixels(pixels, n);

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
}