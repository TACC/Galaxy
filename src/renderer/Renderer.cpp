// ========================================================================== //
// Copyright (c) 2014-2018 The University of Texas at Austin.                 k/
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

#define _GNU_SOURCE // XXX TODO: what needs this? remove if possible
#include <sched.h>

#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <math.h>

#include <ospray/ospray.h>

#include "galaxy.h"

#include "Application.h"
#include "Camera.h"
#include "DataObjects.h"
#include "Datasets.h"
#include "Pixel.h"
#include "RayFlags.h"
#include "RayQManager.h"
#include "Renderer.h"
#include "TraceRays.h"
#include "Work.h"
#include "Threading.h"

#include "Rays_ispc.h"
#include "TraceRays_ispc.h"
#include "Visualization_ispc.h"

#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"

#ifdef GXY_TIMING
#include "Timing.h"
static Timer timer("ray_processing");
#endif

using namespace rapidjson;
using namespace std;

namespace gxy
{
WORK_CLASS_TYPE(Renderer::RenderMsg);
WORK_CLASS_TYPE(Renderer::StatisticsMsg);
WORK_CLASS_TYPE(Renderer::SendRaysMsg);
WORK_CLASS_TYPE(Renderer::SendPixelsMsg);

#ifdef GXY_SYNCHRONOUS
WORK_CLASS_TYPE(Renderer::AckRaysMsg);
#endif // GXY_SYNCHRONOUS

KEYED_OBJECT_TYPE(Renderer)

Renderer *Renderer::theRenderer;

void
Renderer::Initialize()
{
  RegisterClass();
  
  RegisterDataObjects();
  
  Camera::Register();
  Rendering::Register();
  RenderingSet::Register();
 
  RenderMsg::Register();
  SendRaysMsg::Register();
  SendPixelsMsg::Register();
  StatisticsMsg::Register();

#ifdef GXY_SYNCHRONOUS
  AckRaysMsg::Register();
#endif // GXY_SYNCHRONOUS

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

	max_rays_per_packet = getenv("GXY_RAYS_PER_PACKET") ? atoi(getenv("GXY_RAYS_PER_PACKET")) : 1000000;
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
#ifdef GXY_EVENT_TRACKING
	GetTheEventTracker()->Add(new StartRenderingEvent);
#endif
#ifdef GXY_LOGGING
	APP_LOG(<< "Renderer::localRendering start");
#endif

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
	
	int fnum = renderingSet->NeedInitialRays();
  if (fnum != -1)
	{
	  // std::cerr << GetTheApplication()->GetRank() << " starting " << frame << "\n";
#ifdef GXY_LOGGING
	if (GetTheApplication()->GetRank() == 0)
		std:cerr << "starting ray processing\n";
#endif

#ifdef GXY_SYNCHRONOUS
		GetTheRayQManager()->Pause();

		if (renderingSet->CameraIsActive())
		{
			std::cerr << "RenderingSet: Cannot InitializeState when camera is already active\n";
			exit(0);
		}
	
		// Bump to keep from seeming finished until all camera rays have been spawned

		renderingSet->IncrementActiveCameraCount();

		GetTheRayQManager()->Resume();
#endif

#ifdef GXY_PRODUCE_STATUS_MESSAGES
		renderingSet->_initStateTimer();
#endif

#ifdef GXY_SYNCHRONOUS
		renderingSet->initializeSpawnedRayCount();
#endif

		vector<future<int>> rvec;
		for (int i = 0; i < renderingSet->GetNumberOfRenderings(); i++)
		{
			RenderingP rendering = renderingSet->GetRendering(i);
			rendering->resolve_lights();

			CameraP camera = rendering->GetTheCamera();
			VisualizationP visualization = rendering->GetTheVisualization();
			Box *gBox = visualization->get_global_box();
			Box *lBox = visualization->get_local_box();

			camera->generate_initial_rays(renderingSet, rendering, lBox, gBox, rvec, fnum);
		}

#ifdef GXY_PRODUCE_STATUS_MESSAGES
		renderingSet->_dumpState(c, "status"); // Note this will sync after cameras, I think
#endif

#ifdef GXY_EVENT_TRACKING
		GetTheEventTracker()->Add(new CameraLoopEndEvent);
#endif

#ifdef GXY_SYNCHRONOUS
		for (auto& r : rvec)
			r.get();

		int global_spawned_ray_count, local_spawned_ray_count = renderingSet->getSpawnedRayCount();
		if (GetTheApplication()->GetTheMessageManager()->UsingMPI())
			MPI_Allreduce(&local_spawned_ray_count, &global_spawned_ray_count, 1, MPI_INT, MPI_SUM, c);
		else
			global_spawned_ray_count = local_spawned_ray_count;

		if (global_spawned_ray_count == 0)
			renderingSet->Finalize();
		else
			renderingSet->DecrementActiveCameraCount(0);
#endif // GXY_SYNCHRONOUS
	}
	
}

void
Renderer::LoadStateFromDocument(Document& doc)
{
	Value& v = doc["Renderer"];

	if (v.HasMember("Lighting"))
		lighting.LoadStateFromValue(v["Lighting"]);
	else if (v.HasMember("lighting"))
		lighting.LoadStateFromValue(v["lighting"]);

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

class processRays_task : public ThreadPoolTask
{
public:
	processRays_task(RayList *raylist, Renderer *renderer) : 
		ThreadPoolTask(raylist->GetType() == RayList::PRIMARY ? 1 : 0), raylist(raylist), renderer(renderer) {}
  ~processRays_task() {}

	int work() { 

		RenderingSetP  renderingSet  = raylist->GetTheRenderingSet();
		RenderingP     rendering     = raylist->GetTheRendering();
		VisualizationP visualization = rendering->GetTheVisualization();

		// This is called when a list of rays is pulled off the
		// RayQ.  When we are done with it we decrement the 
		// ray list count (rather than when it was pulled off the
		// RayQ) so we don't send a message upstream saying we are idle
		// until we actually are.

		while (raylist)
		{
		  if (raylist->GetFrame() != renderingSet->GetCurrentFrame())
		  {
				// std::cerr << GetTheApplication()->GetRank() << " dropping raylist (" << raylist->GetFrame() << ", " <<  renderingSet->GetCurrentFrame() << ")\n";
				delete raylist;
				return 0;
		  }
			// else
				// std::cerr << GetTheApplication()->GetRank() << " processing raylist " << raylist->GetRayCount() << "\n";

			// This may put secondary lists on the ray queue
			renderer->Trace(raylist);

			// Tracing the input rays classifies them by termination class.
			// If a ray is terminated by a boundary, it needs to get moved
			// to the corresponding neighbor (unless there is none, in which 
			// case it is just plain terminated).   If it is just plain
			// terminated, add its contribution to the FB

			Box *box = visualization->get_local_box();

			// Where does each ray go next?  Either nowhere (DROP_ON_FLOOR), into one of
			// the 0->5 neighbors, to the FB (SEND_TO_FB) or remain here (KEEP_HERE)

			int *classification = new int[raylist->GetRayCount()];

			for (int i = 0; i < raylist->GetRayCount(); i++)
			{
				int typ  = raylist->get_type(i);
				int term = raylist->get_term(i);

				int exit_face; // Which face a ray that terminated at a boundary
											 // exits. NO_NEIGHBOR if the boundary is an external 
											 // boundary.   Only matters if ray hit boundaty

				if (term & RAY_BOUNDARY)
				{
					// then its an internal boundary between 
					// blocks or an external boundary. Figure out
					// which

					exit_face = box->exit_face(raylist->get_ox(i), raylist->get_oy(i), raylist->get_oz(i),
																		 raylist->get_dx(i), raylist->get_dy(i), raylist->get_dz(i));

					if (! visualization->has_neighbor(exit_face)) 
						exit_face = NO_NEIGHBOR;
				}

				if (typ == RAY_PRIMARY)
				{
					// A primary ray expires and is added to the FB if it terminated opaque OR if 
					// it has timed out OR if it exitted the global box. 
					//
					// If its opaque it has a non-zero color component.  If it times out, it has a 
					// non-zero lighting component.  In either case, we send it to the FB
					//
					// If it exits the global box, we only send it if it has a non-zero color component
					//
					// Otherwise, if it hit a *partition* boundary then it'll go to the neighbor.
					//
					// Otherwise, it better have hit a TRANSLUCENT  surface and will remain in the
					// current partition.

					if ((term & RAY_OPAQUE) | (term & RAY_TIMEOUT))
					{
						classification[i] = SEND_TO_FB;
					}
					else if ((term & RAY_BOUNDARY) && (exit_face == NO_NEIGHBOR))
					{
						if ((raylist->get_r(i) != 0) || (raylist->get_g(i) != 0) || (raylist->get_b(i) != 0))
							classification[i] = SEND_TO_FB;
						else
							classification[i] = DROP_ON_FLOOR;
					}
					else if (term & RAY_BOUNDARY)  // then exit_face is not NO_NEIGHBOR -- see previous condition
					{
						classification[i] = exit_face;  // 0 ... 5
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
							// OR, if reverse lighting, drop on floor - the ray escaped and so we don't want
							// to ADD the (negative) shadow

#ifdef GXY_REVERSE_LIGHTING
							classification[i] = DROP_ON_FLOOR;
#else
							classification[i] = SEND_TO_FB;
#endif
						}
						else
							classification[i] = exit_face;  // 0 ... 5
					}
					else if ((term & RAY_OPAQUE) | (term & RAY_SURFACE))
					{
						// We don't need to add in the light's contribution
						// OR, if reverse lighting, send to FB to ADD the (negative) shadow
#ifdef GXY_REVERSE_LIGHTING
					classification[i] = SEND_TO_FB;
#else
          classification[i] = DROP_ON_FLOOR;
#endif
					}
					else 
					{
						std::cerr << "Shadow ray died for an unknown reason\n";
						classification[i] = DROP_ON_FLOOR;
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

#ifdef GXY_REVERSE_LIGHTING
							classification[i] = DROP_ON_FLOOR;
#else
							classification[i] = SEND_TO_FB;
#endif
						}
						else
							classification[i] = exit_face;	// 0 ... 5
					}
					else if ((term & RAY_OPAQUE) | (term & RAY_SURFACE))
					{
						// We don't need to add in the light's contribution
						// OR, if reverse lighting, send to FB to ADD the (negative)
						// ambient contribution
#ifdef GXY_REVERSE_LIGHTING
						classification[i] = SEND_TO_FB;
#else
						classification[i] = DROP_ON_FLOOR;
#endif
					}
					else if (term == RAY_TIMEOUT)
					{
						// Timed-out - drop on floor in REVERSE case so
						// we don't add the (negative) ambient contribution;
						// otherwise, send  to FB to add in (positive) ambient
						// contribution
#ifdef GXY_REVERSE_LIGHTING
						classification[i] = DROP_ON_FLOOR;
#else
						classification[i] = SEND_TO_FB;
#endif
					}
					else
					{
						std::cerr << "AO ray died for an unknown reason\n";
						classification[i] = -1;
					}
				}
			}

			// OK, now we know the fate of the input rays.   Partition them accordingly
			// counts going to each destination - six exit faces and stay right here.

			int knts[7] = {0, 0, 0, 0, 0, 0, 0};

			int fbknt = 0;
			for (int i = 0; i < raylist->GetRayCount(); i++)
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

			// Allocate a RayList for each remote destination's rays and the keepers.
			// Note that // there'll never be more rays being passed to a neighbor from 
			// the current ray list than the number of rays *in* the current ray list, 
			// so we don't have to worry about these being larger than the rays-per-packet
			// limit.
			//
			// They'll bound up in a SendRays message later, without a copy. 7 lists, for
			// each of 6 faces and stay here

			RayList *ray_lists[7];
			for (int i = 0; i < 7; i++)
			{
				if (knts[i])
					ray_lists[i]   = new RayList(renderingSet, rendering, knts[i], raylist->GetFrame(), raylist->GetType());
				else
					ray_lists[i]   = NULL;

				knts[i] = 0;   // We'll be re-using this to keep track of where the next ray goes in
											 // the output ray list
			}


			// Now we sort the rays based on the classification.   If SEND_TO_FB
			// and the FB is local,  just do it.  Otherwise, set up a message buffer.

			RenderingP rendering = raylist->GetTheRendering();

			Renderer::SendPixelsMsg *spmsg = (fbknt && !rendering->IsLocal()) ? 
						new Renderer::SendPixelsMsg(rendering, renderingSet, raylist->GetFrame(), fbknt) : NULL;

			Pixel *local_pixels = (fbknt && rendering->IsLocal()) ? new Pixel[fbknt] : NULL;

			fbknt = 0;
			int lclknt = 0;
			for (int i = 0; i < raylist->GetRayCount(); i++)
			{
				int clss = classification[i];
				if (clss >= 0 && clss <= 6)
				{
					if (knts[clss] >= ray_lists[clss]->GetRayCount())
						std::cerr << "CLASSIFICATION ERROR 2\n";

					RayList::CopyRay(raylist, i, ray_lists[clss], knts[clss]);
					knts[clss]++;
				}
				else if (clss == SEND_TO_FB)
				{
					if (rendering->IsLocal())
					{
						Pixel *p = local_pixels + lclknt;

						p->x = raylist->get_x(i);
						p->y = raylist->get_y(i);
						p->r = raylist->get_r(i);
						p->g = raylist->get_g(i);
						p->b = raylist->get_b(i);
						p->o = raylist->get_o(i);

						lclknt++;
					}
					else
					{
						spmsg->StashPixel(raylist, i);
						fbknt++;
					}
				}
			}

			// If we AREN't the owner of the Rendering send the pixels to its owner
			if (spmsg)
			{
			  if (raylist->GetFrame() == renderingSet->GetCurrentFrame())
				{
					// std::cerr << GetTheApplication()->GetRank() << " sending pixellist to owner " << rendering->GetTheOwner() << "\n";
				  spmsg->Send(rendering->GetTheOwner());
				}
				else
				{
					// std::cerr << GetTheApplication()->GetRank() << " not sending pixellist (" << raylist->GetFrame() << ", " <<  renderingSet->GetCurrentFrame() << ")\n";
				  delete spmsg;
				}
			}

			if (local_pixels)
			{
				rendering->AddLocalPixels(local_pixels, lclknt, raylist->GetFrame(), GetTheApplication()->GetRank());
				delete[] local_pixels;
			}

			for (int i = 0; i < 6; i++)
				if (knts[i])
				{
#ifdef GXY_SYNCHRONOUS
					// This process gets "ownership" of the new ray list until its recipient acknowleges 
					renderingSet->IncrementRayListCount();
					renderer->SendRays(ray_lists[i], visualization->get_neighbor(i));
#else
					if (ray_lists[i]->GetFrame() == renderingSet->GetCurrentFrame())
					{
						renderer->SendRays(ray_lists[i], visualization->get_neighbor(i));
					}
					// else
						// std::cerr << GetTheApplication()->GetRank() << " not sending raylist (" << raylist->GetFrame() << ", " <<  renderingSet->GetCurrentFrame() << ")\n";
#endif // GXY_SYNCHRONOUS

					delete ray_lists[i];
				}

			if (classification) delete[] classification;

			delete raylist;

			// Just loop on the keepers.

			raylist = ray_lists[6];
		}

#ifdef GXY_SYNCHRONOUS
		// Finished processing this ray list.  
		renderingSet->DecrementRayListCount();
#endif //  GXY_SYNCHRONOUS

		return 0;
	}

private:
	RayList *raylist;
	Renderer *renderer;
};

void
Renderer::Trace(RayList *raylist)
{
	RenderingSetP  renderingSet  = raylist->GetTheRenderingSet();
	RenderingP     rendering     = raylist->GetTheRendering();
	VisualizationP visualization = rendering->GetTheVisualization();

	// This is called when a list of rays is pulled off the
	// RayQ.  When we are done with it we decrement the 
	// ray list count (rather than when it was pulled off the
	// RayQ) so we don't send a message upstream saying we are idle
	// until we actually are.

	RayList *out = tracer.Trace(rendering->GetLighting(), visualization, raylist);
	if (out)
	{
		if (out->GetRayCount() > Renderer::GetTheRenderer()->GetMaxRayListSize())
		{
			vector<RayList*> rayLists;
			out->Split(rayLists);
			for (vector<RayList*>::iterator it = rayLists.begin(); it != rayLists.end(); it++)
			{
				RayList *s = *it;
				renderingSet->Enqueue(*it);
			}
			delete out;
		}
		else
			renderingSet->Enqueue(out);
	}
}

void
Renderer::ProcessRays(RayList *in)
{
	GetTheApplication()->GetTheThreadPool()->AddTask(new processRays_task(in, this));
}

void
Renderer::_dumpStats()
{
	stringstream s;
	s << endl << "originated ray count " << originated_ray_count << " rays" << endl;
	s << "sent ray count " << sent_ray_count << " rays" << endl;
	s << "terminated ray count " << terminated_ray_count << " rays" << endl;
	s << "secondary ray count " << secondary_ray_count << " rays" << endl;
	s << "ProcessRays saw " << ProcessRays_input_count << " rays" << endl;
	s << "ProcessRays continued " << ProcessRays_continued_count << " rays" << endl;
	s << "ProcessRays sent " << sent_pixels_count << " pixels" << endl;
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
#ifdef GXY_SYNCHRONOUS
	rays->GetTheRenderingSet()->IncrementInFlightCount();
#endif

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
		cerr << "WARNING: ray list arrived before rendering/renderingSet" << endl;
		delete rayList;
		return false;
	}

#ifdef GXY_EVENT_TRACKING
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

#ifdef GXY_SYNCHRONOUS
	Renderer::AckRaysMsg ack(renderingSet);
	ack.Send(sender);
#endif // GXY_SYNCHRONOUS
	
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

#ifdef GXY_SYNCHRONOUS
Renderer::AckRaysMsg::AckRaysMsg(RenderingSetP rs) : AckRaysMsg(sizeof(Key))
{
	*(Key *)contents->get() = rs->getkey();
}

bool
Renderer::AckRaysMsg::Action(int sender)
{
	RenderingSetP rs = RenderingSet::GetByKey(*(Key *)contents->get());
	rs->DecrementInFlightCount();
	rs->DecrementRayListCount();
	return false;
}
#endif // GXY_SYNCHRONOUS

void
Renderer::Render(RenderingSetP rs)
{
	static int render_frame = 0;
#ifdef GXY_EVENT_TRACKING

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
  msg.Broadcast(true, true);
}

void Renderer::DumpStatistics()
{
  StatisticsMsg *s = new StatisticsMsg(this);
  s->Broadcast(true, true);
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

} // namespace gxy
