#include <iostream>
#include <sstream>
#include <fstream>
#include <math.h>

#include <ospray/ospray.h>

#include "galaxy.h"
#include "Application.h"
#include "Work.h"
#include "Renderer.h"
#include "RayFlags.h"
#include "Pixel.h"
#include "TraceRays.h"
#include "Datasets.h"
#include "Camera.h"
#include "RayQManager.h"
#include "Lighting.h"
#include "DataObjects.h"

#include "Visualization_ispc.h"
#include "Rays_ispc.h"
#include "TraceRays_ispc.h"

#ifdef GXY_TIMING
#include "Timing.h"
static Timer timer("ray_processing");
#endif

#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"

using namespace rapidjson;
using namespace std;

namespace gxy
{
WORK_CLASS_TYPE(Renderer::RenderMsg);
WORK_CLASS_TYPE(Renderer::SendRaysMsg);
WORK_CLASS_TYPE(Renderer::SendPixelsMsg);
WORK_CLASS_TYPE(Renderer::AckRaysMsg);

KEYED_OBJECT_TYPE(Renderer)

void
Renderer::Initialize()
{
  //std::cerr << "Renderer::Initialize()\n";
  
  RegisterClass();
  
  RegisterDataObjects();
  
  Camera::Register();
  Rendering::Register();
  RenderingSet::Register();
 
  RenderMsg::Register();
  SendRaysMsg::Register();
  SendPixelsMsg::Register();
  AckRaysMsg::Register();
}

void
Renderer::initialize()
{
  rayQmanager = new RayQManager(this);
}

Renderer::~Renderer()
{
  	rayQmanager->Kill();
    delete rayQmanager;
 }

void 
Renderer::SetEpsilon(float e)
{ 
  tracer.SetEpsilon(e); 
}

void
Renderer::localRendering(RenderingSetP rs, MPI_Comm c)
{
#ifdef GXY_LOGGING
	APP_LOG(<< "Renderer::localRendering start");
#endif

	// This will prevent any rays from being dequeued until the initial state is sent up the tree
	// That'll happen in the InitialState exchange

	//Renderer::GetTheRenderer()->GetTheRayQManager()->Pause();

  GetTheRayQManager()->Pause();
  
	vector<future<void>> rvec;
	for (int i = 0; i < rs->GetNumberOfRenderings(); i++)
		LaunchInitialRays(rs->GetRendering(i), rvec);

	for (auto& r : rvec)
		r.get();

	MPI_Barrier(c);

#ifdef GXY_LOGGING
	if (GetTheApplication()->GetRank() == 0)
		std:cerr << "starting ray processing\n";
#endif
  
	rs->InitialState(c);   // This will resume the ray q
}

void
Renderer::LaunchInitialRays(RenderingP rendering, vector<future<void>>& rvec)
{
  CameraP camera = rendering->GetTheCamera();
  VisualizationP visualization = rendering->GetTheVisualization();
  Box *gBox = visualization->get_global_box();
  Box *lBox = visualization->get_local_box();

	camera->generate_initial_rays(rendering, lBox, gBox, rvec);
}

void
Renderer::LoadStateFromDocument(Document& doc)
{
	Value& v = doc["Renderer"];

	if (v.HasMember("Lighting"))
		lighting.LoadStateFromValue(v["Lighting"]);

	if (v.HasMember("Tracer"))
		tracer.LoadStateFromValue(v["Tracer"]);
}

void
Renderer::SaveStateToDocument(Document& doc)
{
  Value r(kObjectType);

  lighting.SaveStateToValue(r, doc);
  tracer.SaveStateToValue(r, doc);

	doc.AddMember("Renderer", r, doc.GetAllocator());
}

class LocalPixelContributionsEvent : public Event
{
public:
	LocalPixelContributionsEvent(int c, RenderingP r) : count(c), rset(r->GetTheRenderingSetKey()) { }

protected:
	void print(ostream& o)
	{
		Event::print(o);
		o << count << " pixels handled locally for rset " << rset;
	}

private:
	int count;
	Key rset;
};

class RemotePixelContributionsEvent : public Event
{
public:
	RemotePixelContributionsEvent(int c, RenderingP r) : count(c), dest(r->GetTheOwner()),  rset(r->GetTheRenderingSetKey()) {}

protected:
	void print(ostream& o)
	{
		Event::print(o);
		o << count << " pixels sent to " << dest << " for rset " << rset;
	}

private:
	int count;
	int dest;
	Key rset;
};

class ProcessRayListEvent : public Event
{
public:
	ProcessRayListEvent(int ni, int nn, int nr, int ns) : nIn(ni), nNew(nn), nRetired(nr), nSent(ns) {}

protected:
	void print(ostream& o)
	{
		Event::print(o);
		o << " ProcessRayList:  " << nIn << " in, " << nNew << " spawned, " << nRetired << " retired and " << nSent << " sent";
	}

private:
	int nIn, nNew, nSent, nRetired;
};

// These defines categorize rays after a pass through the tracer

#define SEND_TO_FB			-1
#define DROP_ON_FLOOR		-2
#define KEEP_HERE				-3
#define UNDETERMINED		-4

// This define indicates that a ray exitting a partition face exits everything

#define NO_NEIGHBOR 		-1

int xyzzy = -1;
int xyzzy1 = -1;

void
Renderer::ProcessRays(RayList *in)
{
  //std::cerr << "in list: " << in->get_header_address() << "\n";
  
#ifdef GXY_TIMING
  timer.start();
#endif

	bool first = true;

	int nIn = in->GetRayCount();
	int nRetired = 0;
	int nNew		 = 0;
	int nSent    = 0;


	RenderingP rendering = in->GetTheRendering();
	RenderingSetP renderingSet = rendering->GetTheRenderingSet();
	VisualizationP visualization = rendering->GetTheVisualization();

	// This is called when an list of rays is pulled off the
	// RayQ.  When we are done with it we decrement the 
	// ray list count (rather than when we pulled it off the
	// RayQ so we don't send a message upstream saying we are idle
	// until we actually are.
	// we remain busy 

	while (in)
	{
		RayList *out = tracer.Trace(lighting, visualization, in);
    if (out)
       //std::cerr << "out list: " << out->get_header_address() << "\n";

		// the result are the generated rays which by definition begin
		// here, so we enqueue them for local processing - again, not silent

		if (out)
		{
				nNew += out->GetRayCount();

#if defined(EVENT_TRACKING)
				class TraceRaysEvent : public Event
				{
				public:
					TraceRaysEvent(int n, int p, int s, Key r) : nin(n), phits(p), secondaries(s), rset(r) {};

				protected:
					void print(ostream& o)
					{
						Event::print(o);
						o << "TraceRays processed " << nin << " rays with " << phits << " hits generating " << secondaries << " secondaries for set " << rset;
					}

					private:
						int nin;
						int phits;
						int secondaries;
						Key rset;
				};

				int nHits = 0;
				for (int i = 0; i < in->GetRayCount(); i++)
				{
					int typ  = in->type(i);
					int term = in->term(i);
					if ((typ(i) == RAY_PRIMARY) && (term & RAY_SURFACE)) nHits++;
				}

				GetTheEventTracker()->Add(new TraceRaysEvent(in->GetRayCount(), nHits, out->GetRayCount(), out->GetTheRendering()->GetTheRenderingSetKey()));
#endif
				renderingSet->Enqueue(out);
			}

#ifdef GXY_LOGGING
		if (out)
			APP_LOG(<< out->GetRayCount() << " secondaries re-enqueued");
#endif
		
		// Tracing the input rays classifies them by termination class.
		// If a ray is terminated by a boundary, it needs to get moved
		// to the corresponding neighbor (unless there is none, in which 
		// case it is just plain terminated).   If it is just plain
		// terminated, add its contribution to the FB

		Box *box = visualization->get_local_box();

		// Where does each ray go next?  Either nowhere (DROP_ON_FLOOR), into one of
		// the 0->5 neighbors, to the FB (SEND_TO_FB) or remain here (KEEP_HERE)

		int *classification = new int[in->GetRayCount()];

    for (int i = 0; i < in->GetRayCount(); i++)
    {
		  int typ  = in->get_type(i);
			int term = in->get_term(i);

      int exit_face; // Which face a ray that terminated at a boundary
							 			 // exits. NO_NEIGHBOR if the boundary is an external 
										 // boundary.   Only matters if ray hit boundaty

      if (term & RAY_BOUNDARY)
      {
        // then its an internal boundary between 
        // blocks or an external boundary. Figure out
        // which

        exit_face = box->exit_face(in->get_ox(i), in->get_oy(i), in->get_oz(i), in->get_dx(i), in->get_dy(i), in->get_dz(i));

        if (! visualization->has_neighbor(exit_face)) 
          exit_face = NO_NEIGHBOR;
      }

      if (typ == RAY_PRIMARY)
      {
        // A primary ray expires and is added to the FB if it terminated opaque OR if 
        // it has timed out OR if it exitted the global box. 
        //
        // Otherwise, if it hit a *partition* boundary then it'll go to the neighbor.
        //
        // Otherwise, it better have hit a TRANSLUCENT  surface and will remain in the
        // current partition.

        if ((term & RAY_OPAQUE) | ((term & RAY_BOUNDARY) && exit_face == NO_NEIGHBOR) | (term & RAY_TIMEOUT))
        {
          classification[i] = SEND_TO_FB;
					nRetired ++;
        }
        else if (term & RAY_BOUNDARY)  // then exit_face is not NO_NEIGHBOR
        {
          classification[i] = exit_face;  // 0 ... 5
					nSent ++;
        }
        else		// Translucent surface
        {
					classification[i] = KEEP_HERE;
        }
      }
    
      // If its a shadow ray it gets added into the FB if it terminated at the 
      // external boundary.   If it exitted at a internal boundary it keeps 
      // going.   If it hit a surface it dies.

      else if (typ == RAY_SHADOW)
      {
        if (term & RAY_BOUNDARY)
        {
          // If an EXTERNAL boundary....
          if (exit_face == NO_NEIGHBOR)
          {
            // Then send the rays to the FB to add in the directed-light contribution
            classification[i] = SEND_TO_FB;
						nRetired ++;
          }
          else
					{
            classification[i] = exit_face;  // 0 ... 5
						nSent ++;
					}
        }
        else if ((term & RAY_OPAQUE) | (term & RAY_SURFACE))
        {
          // We don't need to add in the light's contribution
          classification[i] = DROP_ON_FLOOR;
					nRetired ++;
        }
        else 
        {
          std::cerr << "Shadow ray died for an unknown reason\n";
          classification[i] = DROP_ON_FLOOR;
					nRetired ++;
        }
      }

      // If its an AO ray, same thing - except it CAN time out.
      
      else if (typ == RAY_AO)
      {
        if (term & RAY_BOUNDARY)
        {
          if (exit_face == NO_NEIGHBOR)
          {
            // Then send the rays to the FB to add in the ambient-light contribution
            classification[i] = SEND_TO_FB;
						nRetired ++;
          }
          else
					{
            classification[i] = exit_face;	// 0 ... 5
						nSent ++;
					}
        }
        else if ((term & RAY_OPAQUE) | (term & RAY_SURFACE))
        {
					classification[i] = DROP_ON_FLOOR;
					nRetired ++;
        }
        else if (term == RAY_TIMEOUT)
        {
					classification[i] = SEND_TO_FB;
					nRetired ++;
				}
				else
				{
					std::cerr << "AO ray died for an unknown reason\n";
					classification[i] = -1;
					nRetired ++;
				}
			}
		}

		// OK, now we know the fate of the input rays.   Partition them accordingly
		// counts going to each destination - six exit faces and stay right here.

		int knts[7] = {0, 0, 0, 0, 0, 0, 0};

		int fbknt = 0;
		for (int i = 0; i < in->GetRayCount(); i++)
		{
			int clss = classification[i];
			if (clss == SEND_TO_FB)
				fbknt++;
			else if (clss == KEEP_HERE)
				knts[6] ++;
			else if (clss >= 0 && clss < 6)
				knts[clss] ++;
			else if (clss != DROP_ON_FLOOR)
				std::cerr << "CLASSIFICATION ERROR 1\n";
		}

		// Allocate a RayList for each remote destination's rays   These'll get
		// bound up in a SendRays message later, without a copy.   7 lists, for
		// each of 6 faces and stay here

		RayList *ray_lists[7];
		int     *ray_offsets[7];
		Ray     *rays[7];
		for (int i = 0; i < 7; i++)
		{
			if (knts[i])
			{
				ray_offsets[i] = new int[in->GetRayCount()];
				ray_lists[i]   = new RayList(rendering, knts[i]);
			}
			else
			{
				ray_lists[i]   = NULL;
				ray_offsets[i] = NULL;
			}
			
			knts[i] = 0;
		}

		// Now we sort the rays based on the classification.   If SEND_TO_FB
		// and the FB is local,  just do it.  Otherwise, set up a message buffer.

		RenderingP rendering = in->GetTheRendering();
		SendPixelsMsg *spmsg = (fbknt && !rendering->IsLocal()) ? new SendPixelsMsg(rendering, fbknt) : NULL;

		fbknt = 0;
		int lclknt = 0;
		for (int i = 0; i < in->GetRayCount(); i++)
		{
			int clss = classification[i];
			if (clss >= 0 && clss <= 6)
			{
				if (knts[clss] >= ray_lists[clss]->GetRayCount())
					std::cerr << "CLASSIFICATION ERROR 2\n";

				RayList::CopyRay(in, i, ray_lists[clss], knts[clss]);
			  knts[clss]++;
			}
			else if (clss == SEND_TO_FB)
			{
				if (rendering->IsLocal())
				{
					rendering->AddLocalPixel(in, i);
					lclknt++;
				}
        else
				{
					spmsg->StashPixel(in, i);
          fbknt++;
				}
			}
		}

#if defined(EVENT_TRACKING)

		if (lclknt)
			GetTheEventTracker()->Add(new LocalPixelContributionsEvent(lclknt, rendering));

		if (fbknt)
			GetTheEventTracker()->Add(new RemotePixelContributionsEvent(fbknt, rendering));

#endif

   // std::cerr << "nRet " << nRetired << " lclknt " << lclknt << " fbknt " << fbknt << "\n";

  // If we AREN't the owner of the Rendering send the pixels to its owner

    if (spmsg)
    {
      spmsg->Send(rendering->GetTheOwner());
      rendering->GetTheRenderingSet()->SentPixels(fbknt);
    }

		for (int i = 0; i < 6; i++)
			if (knts[i])
			{
				// This process gets "ownership" of the new ray list until its recipient acknowleges 
				renderingSet->IncrementRayListCount();
				
				SendRays(ray_lists[i], visualization->get_neighbor(i));
				delete ray_lists[i];
				delete[] ray_offsets[i];
			}

		if (classification) delete[] classification;

		if (ray_offsets[6]) delete[] ray_offsets[6];
		delete in;

		in = ray_lists[6];
	}

#if defined(EVENT_TRACKING)
	GetTheEventTracker()->Add(new ProcessRayListEvent(nIn, nNew, nRetired, nSent));
#endif

	// Finished processing this ray list.  
	renderingSet->DecrementRayListCount();

#ifdef GXY_TIMING
  timer.stop();
#endif
}

#if defined(EVENT_TRACKING)

class SendRaysEvent : public Event
{
public:
  SendRaysEvent(int d, int c, Key r) : dst(d), count(c), rset(r) {};

protected:
  void print(ostream& o)
  {
    Event::print(o);
    o << "send " << count << " rays to "   << dst << " rset " << rset;
  }

private:
  int dst;
  int count;
  Key rset;
};

#endif

void 
Renderer::SendRays(RayList *rays, int destination)
{
#if defined(EVENT_TRACKING)
	GetTheEventTracker()->Add(new SendRaysEvent(destination,  rays->GetRayCount(), rays->GetTheRendering()->GetTheRenderingSetKey()));
#endif
	SendRaysMsg msg(rays);
	msg.Send(destination);
}

bool
Renderer::SendRaysMsg::Action(int sender)
{
	RayList *rayList = new RayList(this->contents);

#if defined(EVENT_TRACKING)
	class ReceiveRaysEvent : public Event
	{
	public:
		ReceiveRaysEvent(int s, int c, Key r) : sender(s), count(c), rset(r) {};

	protected:
		void print(ostream& o)
		{
			Event::print(o);
			o << "received " << count << " rays from " << sender << " for rset " << rset;
		}

	private:
		int sender;
		int count;
		Key rset;
	};

	GetTheEventTracker()->Add(new ReceiveRaysEvent(sender, rayList->GetRayCount(), rayList->GetTheRendering()->GetTheRenderingSetKey()));
#endif

	RenderingP rendering = rayList->GetTheRendering();
	RenderingSetP renderingSet = rendering->GetTheRenderingSet();

	renderingSet->Enqueue(rayList);

	Renderer::AckRaysMsg ack(renderingSet);
	ack.Send(sender);
	
	return false;
}

int
Renderer::SerialSize()
{
	return tracer.SerialSize() + lighting.SerialSize();
}

unsigned char *
Renderer::Serialize(unsigned char *p)
{
	p = lighting.Serialize(p);
	p = tracer.Serialize(p);
	return p;
}

unsigned char *
Renderer::Deserialize(unsigned char *p)
{
	p = lighting.Deserialize(p);
	p = tracer.Deserialize(p);
	return p;
}

Renderer::AckRaysMsg::AckRaysMsg(RenderingSetP rs) : AckRaysMsg(sizeof(Key))
{
	*(Key *)contents->get() = rs->getkey();
}

bool
Renderer::AckRaysMsg::Action(int sender)
{
	RenderingSetP rs = RenderingSet::GetByKey(*(Key *)contents->get());
	rs->DecrementRayListCount();
	return false;
}

void
Renderer::Render(RenderingSetP rs)
{
#if defined(EVENT_TRACKING)

    class StartRenderingEvent : public Event
    {
    public:
      StartRenderingEvent(Key r) : rset(r) {};

    protected:
      void print(ostream& o)
      {
        Event::print(o);
        o << "start rendering rset " << rset;
      }

    private:
      Key rset;
    };

    GetTheEventTracker()->Add(new StartRenderingEvent(rs->getkey()));
#endif


  RenderMsg msg(this, rs);
  msg.Broadcast(true);
}

Renderer::RenderMsg::~RenderMsg()
{
}

Renderer::RenderMsg::RenderMsg(Renderer* r, RenderingSetP rs) :
  Renderer::RenderMsg(sizeof(Key) + r->SerialSize() + sizeof(Key))
{
  unsigned char *p = contents->get();
  *(Key *)p = r->getkey();
  p = p + sizeof(Key);
  p = r->Serialize(p);
  *(Key *)p = rs->getkey();
}

bool
Renderer::RenderMsg::CollectiveAction(MPI_Comm c, bool isRoot)
{
  unsigned char *p = (unsigned char *)get();
  Key renderer_key = *(Key *)p;
  p += sizeof(Key);
 
  RendererP renderer = Renderer::GetByKey(renderer_key);
  p = renderer->Deserialize(p);
  
  RenderingSetP rs = RenderingSet::GetByKey(*(Key *)p);

  MPI_Barrier(c);
  renderer->localRendering(rs, c);

  return false;
}

}