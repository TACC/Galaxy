#define REVERSE_LIGHTING 1

#define do_timing 0
#define logging 0

#include <iostream>
#include <sstream>
#include <fstream>
#include <math.h>

#include <ospray/ospray.h>

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

#if DO_TIMING
#include "Timing.h"
static Timer timer("ray_processing");
#endif

using namespace std;

#include "../rapidjson/document.h"
#include "../rapidjson/stringbuffer.h"

using namespace rapidjson;

WORK_CLASS_TYPE(Renderer::RenderMsg);
WORK_CLASS_TYPE(Renderer::StatisticsMsg);
WORK_CLASS_TYPE(Renderer::SendRaysMsg);
WORK_CLASS_TYPE(Renderer::SendPixelsMsg);

#ifdef PVOL_SYNCHRONOUS
WORK_CLASS_TYPE(Renderer::AckRaysMsg);
#endif // PVOL_SYNCHRONOUS

KEYED_OBJECT_TYPE(Renderer)

Renderer *Renderer::theRenderer;

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
  StatisticsMsg::Register();
#ifdef PVOL_SYNCHRONOUS
  AckRaysMsg::Register();
#endif // PVOL_SYNCHRONOUS

}

void
Renderer::initialize()
{
  theRenderer = this;

	frame = 0;
  rayQmanager = new RayQManager(this);
	pthread_mutex_init(&lock, NULL);

	sent_to = new int[GetTheApplication()->GetSize()];
	received_from = new int[GetTheApplication()->GetSize()];
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
Renderer::localRendering(RenderingSetP renderingSet, MPI_Comm c)
{
	originated_ray_count = 0;
	sent_ray_count = 0;
	sent_pixels_count = 0;
	terminated_ray_count = 0;
	secondary_ray_count = 0;
  ProcessRays_input_count = 0;
  ProcessRays_continued_count = 0;

	for (int i = 0; i < GetTheApplication()->GetSize(); i++)
		sent_to[i] = 0, received_from[i] = 0;

	// NeedInitialRays tells us whether we need to generate initial rays
	// for the current frame.  It is possible that this RenderingSet has
	// already seen a raylist from a later frame, in which case we won't
	// bother generating rays for this one.
	
	if (renderingSet->NeedInitialRays())
	{
#ifdef PVOL_SYNCHRONOUS
		GetTheRayQManager()->Pause();
#endif

		vector<future<void>> rvec;
		for (int i = 0; i < renderingSet->GetNumberOfRenderings(); i++)
		{
			RenderingP rendering = renderingSet->GetRendering(i);

			CameraP camera = rendering->GetTheCamera();
			VisualizationP visualization = rendering->GetTheVisualization();
			Box *gBox = visualization->get_global_box();
			Box *lBox = visualization->get_local_box();

			camera->generate_initial_rays(renderingSet, rendering, lBox, gBox, rvec);
		}

		for (auto& r : rvec)
			r.get();

#ifdef PVOL_SYNCHRONOUS
		renderingSet->InitializeState(c);   // This will resume the ray q
#endif // PVOL_SYNCHRONOUS
	}
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

// These defines categorize rays after a pass through the tracer

#define SEND_TO_FB			-1
#define DROP_ON_FLOOR		-2
#define KEEP_HERE				-3
#define UNDETERMINED		-4

// This define indicates that a ray exitting a partition face exits everything

#define NO_NEIGHBOR 		-1

void
Renderer::ProcessRays(RayList *in)
{
#if DO_TIMING
  timer.start();
#endif

	bool first = true;
	int nIn = in->GetRayCount();

#if defined(EVENT_TRACKING)
	GetTheEventTracker()->Add(new ProcessRayListEvent(in));
#endif

	int nProcessed   = 0;		// total number of rays processed
	int nSecondaries = 0;		// secondaries generated
	int nRaysSent    = 0;		// number of rays sent to neighbors
	int nPixelsSent  = 0;		// number of pixel contributions sent
	int nErrors      = 0;		// number dropped on floor due to error
	int nKept        = 0;		// number that continued into the next loop
	int nRetired     = 0;   // Number rays that expire without producing
													// a pixel contribtution - shadow or AO rays that hit

	RenderingSetP  renderingSet  = in->GetTheRenderingSet();
	RenderingP     rendering     = in->GetTheRendering();
	VisualizationP visualization = rendering->GetTheVisualization();

	// This is called when an list of rays is pulled off the
	// RayQ.  When we are done with it we decrement the 
	// ray list count (rather than when we pulled it off the
	// RayQ so we don't send a message upstream saying we are idle
	// until we actually are.
	// we remain busy 

	while (in)
	{
		nProcessed += in->GetRayCount();

		RayList *out = tracer.Trace(lighting, visualization, in);

		// the result are the generated rays which by definition begin
		// here, so we enqueue them for local processing - again, not silent

		if (out)
		{
				nSecondaries += out->GetRayCount();
				renderingSet->Enqueue(out);

#if defined(EVENT_TRACKING)
		GetTheEventTracker()->Add(new SecondariesGeneratedEvent(out));
#endif
		}

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
					nPixelsSent++;
        }
        else if (term & RAY_BOUNDARY)  // then exit_face is not NO_NEIGHBOR -- see previous condition
        {
          classification[i] = exit_face;  // 0 ... 5
					nRaysSent ++;
        }
        else		// Translucent surface
        {
					classification[i] = KEEP_HERE;
					nKept++;
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
						// OR, if reverse lighting, drop on floor - the ray escaped and so we don't want
						// to ADD the (negative) shadow

#if REVERSE_LIGHTING
						classification[i] = DROP_ON_FLOOR;
						nRetired ++;
#else
            classification[i] = SEND_TO_FB;
						nPixelsSent ++;
#endif
          }
          else
					{
            classification[i] = exit_face;  // 0 ... 5
						nRaysSent ++;
					}
        }
        else if ((term & RAY_OPAQUE) | (term & RAY_SURFACE))
        {
          // We don't need to add in the light's contribution
					// OR, if reverse lighting, send to FB to ADD the (negative) shadow

#if REVERSE_LIGHTING
					classification[i] = SEND_TO_FB;
					nPixelsSent ++;
#else
          classification[i] = DROP_ON_FLOOR;
					nRetired ++;
#endif
        }
        else 
        {
          std::cerr << "Shadow ray died for an unknown reason\n";
          classification[i] = DROP_ON_FLOOR;
					nErrors ++;
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
						// OR, if reverse lighting, drop on floor - the ray escaped and so we don't want
						// to ADD the (negative) shadow

#if REVERSE_LIGHTING
						classification[i] = DROP_ON_FLOOR;
						nRetired ++;
#else
            classification[i] = SEND_TO_FB;
						nPixelsSent ++;
#endif
          }
          else
					{
            classification[i] = exit_face;	// 0 ... 5
						nRaysSent ++;
					}
        }
        else if ((term & RAY_OPAQUE) | (term & RAY_SURFACE))
        {
          // We don't need to add in the light's contribution
					// OR, if reverse lighting, send to FB to ADD the (negative)
					// ambient contribution
#if REVERSE_LIGHTING
					classification[i] = SEND_TO_FB;
					nPixelsSent ++;
#else
					classification[i] = DROP_ON_FLOOR;
					nRetired ++;
#endif
        }
        else if (term == RAY_TIMEOUT)
        {
					// Timed-out - drop on floor in REVERSE case so
					// we don't add the (negative) ambient contribution;
					// otherwise, send  to FB to add in (positive) ambient
				  // contribution
#if REVERSE_LIGHTING
					classification[i] = DROP_ON_FLOOR;
					nRetired ++;
#else
					classification[i] = SEND_TO_FB;
					nPixelsSent ++;
#endif
				}
				else
				{
					std::cerr << "AO ray died for an unknown reason\n";
					classification[i] = -1;
					nErrors ++;
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
		int     totRaysSent = 0;
		for (int i = 0; i < 7; i++)
		{
			if (knts[i])
			{
				ray_offsets[i] = new int[in->GetRayCount()];
				ray_lists[i]   = new RayList(renderingSet, rendering, knts[i], in->GetFrame());
			}
			else
			{
				ray_lists[i]   = NULL;
				ray_offsets[i] = NULL;
			}
			
			if (i < 6) totRaysSent += knts[i];
			knts[i] = 0;
		}

		if (totRaysSent != nRaysSent)
			std::cerr << "ERROR totRaysSent != nRaysSent\n";

		if (knts[6] != nKept)
			std::cerr << "ERROR knts[6] != nKept\n";
		

		// Now we sort the rays based on the classification.   If SEND_TO_FB
		// and the FB is local,  just do it.  Otherwise, set up a message buffer.

		RenderingP rendering = in->GetTheRendering();
		SendPixelsMsg *spmsg = (fbknt && !rendering->IsLocal()) ? new SendPixelsMsg(rendering, renderingSet, in->GetFrame(), fbknt) : NULL;
		Pixel *local_pixels = (fbknt && rendering->IsLocal()) ? new Pixel[fbknt] : NULL;

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
					Pixel *p = local_pixels + lclknt;

					p->x = in->get_x(i);
					p->y = in->get_y(i);
					p->r = in->get_r(i);
					p->g = in->get_g(i);
					p->b = in->get_b(i);
					p->o = in->get_o(i);

					lclknt++;
				}
        else
				{
					spmsg->StashPixel(in, i);
          fbknt++;
				}
			}
		}

		// If we AREN't the owner of the Rendering send the pixels to its owner
    if (spmsg)
      spmsg->Send(rendering->GetTheOwner());

		if (local_pixels)
		{
			rendering->AddLocalPixels(local_pixels, lclknt, in->GetFrame(), GetTheApplication()->GetRank());
			delete[] local_pixels;
		}

		for (int i = 0; i < 6; i++)
			if (knts[i])
			{
#ifdef PVOL_SYNCHRONOUS
				// This process gets "ownership" of the new ray list until its recipient acknowleges 
				renderingSet->IncrementRayListCount();
#endif // PVOL_SYNCHRONOUS
				
				SendRays(ray_lists[i], visualization->get_neighbor(i));
				delete ray_lists[i];
				delete[] ray_offsets[i];
			}

		if (classification) delete[] classification;
		if (ray_offsets[6]) delete[] ray_offsets[6];

		delete in;

		in = ray_lists[6];
		if (in && in->GetRayCount() > 0)
			std::cerr << "ProcessRays looping\n";
	}

	pthread_mutex_lock(&lock);
	ProcessRays_input_count 		+= nIn;
  sent_ray_count 							+= nRaysSent;
  terminated_ray_count 				+= nRetired;
  secondary_ray_count	  		  += nSecondaries;
  sent_pixels_count           += nPixelsSent;
  ProcessRays_continued_count += nKept;
	pthread_mutex_unlock(&lock);

	if (nProcessed != (nIn + ProcessRays_continued_count)) std::cerr << "ERROR 1: nProcessed != (nIn + ProcessRays_continued_count)\n";
	if (nProcessed != (nRaysSent + nPixelsSent + nRetired)) std::cerr << "ERROR 2: nProcessed != (nRaysSent + nPixelsSent + nRetired)\n";

#if defined(EVENT_TRACKING)
	GetTheEventTracker()->Add(new ProcessRayListCompletedEvent(nIn, nSecondaries, nRetired, nRaysSent));
#endif

#ifdef PVOL_SYNCHRONOUS
	// Finished processing this ray list.  
	renderingSet->DecrementRayListCount();
#endif //  PVOL_SYNCHRONOUS

#if DO_TIMING
  timer.stop();
#endif
}

void
Renderer::_dumpStats()
{
	stringstream s;
	s << "\noriginated ray count " << originated_ray_count << " rays\n";
	s << "sent ray count " << sent_ray_count << " rays\n";
	s << "terminated ray count " << terminated_ray_count << " rays\n";
	s << "secondary ray count " << secondary_ray_count << " rays\n";
	s << "ProcessRays saw " << ProcessRays_input_count << " rays\n";
	s << "ProcessRays continued " << ProcessRays_continued_count << " rays\n";
	s << "ProcessRays sent " << sent_pixels_count << " pixels\n";
	s << "sent to neighbors:";

	for (int i = 0; i < GetTheApplication()->GetSize(); i++)
		s << sent_to[i] << (i == (GetTheApplication()->GetSize() - 1) ? "\n" : " ");

	s << "received from neighbors:";
	for (int i = 0; i < GetTheApplication()->GetSize(); i++)
		s << received_from[i] << (i == (GetTheApplication()->GetSize() - 1) ? "\n" : " ");
	
	APP_LOG(<< s.str());
}

bool
Renderer::StatisticsMsg::CollectiveAction(MPI_Comm c, bool is_root)
{
	char *ptr = (char *)contents->get();
  Key key = *(Key *)ptr;
  RendererP r = GetByKey(key);
	r->_dumpStats();
  return false;
}

void 
Renderer::SendRays(RayList *rays, int destination)
{
	int nReceived = rays->GetRayCount();
	_sent_to(destination, nReceived);

	SendRaysMsg msg(rays);
	msg.Send(destination);
}

bool
Renderer::SendRaysMsg::Action(int sender)
{
	RayList *rayList = new RayList(this->contents);

	int nReceived = rayList->GetRayCount();
	Renderer::GetTheRenderer()->_received_from(sender, nReceived);

	RenderingP rendering = rayList->GetTheRendering();
	RenderingSetP renderingSet = rendering ? rayList->GetTheRenderingSet() : NULL;
	if (! renderingSet)
	{
		std::cerr << "ray list arrived before rendering/renderingSet\n";
		delete rayList;
		return false;
	}

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

	GetTheEventTracker()->Add(new ReceiveRaysEvent(sender, rayList->GetRayCount(), rayList->GetTheRenderingSet()->getkey()));
#endif

	renderingSet->Enqueue(rayList);

#ifdef PVOL_SYNCHRONOUS
	Renderer::AckRaysMsg ack(renderingSet);
	ack.Send(sender);
#endif // PVOL_SYNCHRONOUS
	
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

#ifdef PVOL_SYNCHRONOUS
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
#endif // PVOL_SYNCHRONOUS

void
Renderer::Render(RenderingSetP rs)
{
	static int render_frame = 0;
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

void Renderer::DumpStatistics()
{
  StatisticsMsg *s = new StatisticsMsg(this);
  s->Broadcast(true);
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

  renderer->localRendering(rs, c);

  return false;
}
