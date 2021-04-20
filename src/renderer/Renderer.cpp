// ========================================================================== //
// Copyright (c) 2014-2020 The University of Texas at Austin.                 k/
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

//#define _GNU_SOURCE // XXX TODO: what needs this? remove if possible
#include <sched.h>

#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <math.h>

// #include <ospray/ospray.h>

#include "galaxy.h"

#include "Application.h"
#include "Device.h"
#include "Camera.h"
#include "DataObjects.h"
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
#include "Timer.h"
static gxy::Timer timer("ray_processing");
#endif

using namespace rapidjson;
using namespace std;

namespace gxy
{
WORK_CLASS_TYPE(Renderer::RenderMsg);
WORK_CLASS_TYPE(Renderer::StatisticsMsg);
WORK_CLASS_TYPE(Renderer::SendRaysMsg);
WORK_CLASS_TYPE(Renderer::SendPixelsMsg);

#ifdef GXY_WRITE_IMAGES
WORK_CLASS_TYPE(Renderer::AckRaysMsg);
#endif // GXY_WRITE_IMAGES

OBJECT_CLASS_TYPE(Renderer)

// define class static variables
int Renderer::TERMINATED    = -1;
int Renderer::DROP_ON_FLOOR = -2;
int Renderer::KEEP_HERE     = -3;
int Renderer::UNDETERMINED  = -4;
int Renderer::NO_NEIGHBOR   = -1;

void
Renderer::Initialize()
{
  // extern void force_ospray_library_load();
  // force_ospray_library_load();


  RegisterClass();
  
  RegisterDataObjects();

  DeviceModel::RegisterClass();
  
  Camera::Register();
  Rendering::Register();
  RenderingSet::Register();
 
  RenderMsg::Register();
  SendRaysMsg::Register();
  SendPixelsMsg::Register();
  StatisticsMsg::Register();

  Datasets::Register();

  Vis::Register();
  Visualization::Register();

#ifdef GXY_WRITE_IMAGES
  AckRaysMsg::Register();
#endif // GXY_WRITE_IMAGES
}

#if 0
static  void
print_ospray_error_messages(const char *m)
{
  // APP_LOG(<< m);
  std::cerr << m;
}
#endif

void
Renderer::initialize()
{
  // ospray = GetOspray();
  epsilon = 0.001;

  frame = 0;
  rayQmanager = new RayQManager(this);
  pthread_mutex_init(&lock, NULL);

  sent_to = new int[GetTheApplication()->GetSize()];
  received_from = new int[GetTheApplication()->GetSize()];

  max_rays_per_packet = getenv("GXY_RAYS_PER_PACKET") ? atoi(getenv("GXY_RAYS_PER_PACKET")) : 1000000;

// If writing images, DO NOT set permute pixels sinnce this will reset the permutation table 
// for each generate_pixels that share a camera

#ifdef GXY_WRITE_IMAGES
  SetPermutePixels(false);
#else
  char *permute = getenv("GXY_PERMUTE_PIXELS");
  if (permute)
    SetPermutePixels(atoi(permute) > 0);
  else
    SetPermutePixels(true);
#endif

#if 0
  char *ospMsgs = getenv("GXY_SHOW_OSPRAY_MESSAGES");
  if (ospMsgs && atoi(ospMsgs) > 0)
    ospDeviceSetStatusFunc(ospGetCurrentDevice(), print_ospray_error_messages);
#endif
}

Renderer::~Renderer()
{
    rayQmanager->Kill();
    delete rayQmanager;
}

void 
Renderer::SetEpsilon(float e)
{ 
  epsilon = e;
}

float
Renderer::GetEpsilon()
{ 
  return epsilon;
}

bool
Renderer::local_commit(MPI_Comm c)
{
  if (super::local_commit(c)) return true;
  return false;
}

void
Renderer::local_render(RendererDPtr renderer, RenderingSetDPtr renderingSet)
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
#ifdef GXY_LOGGING
  if (GetTheApplication()->GetRank() == 0)
    std:cerr << "starting ray processing\n";
#endif

#ifdef GXY_WRITE_IMAGES
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

#ifdef GXY_WRITE_IMAGES
    renderingSet->initializeSpawnedRayCount();
#endif

    vector<future<int>> rvec;
    for (int i = 0; i < renderingSet->GetNumberOfRenderings(); i++)
    {
      RenderingDPtr rendering = renderingSet->GetRendering(i);
      rendering->resolve_lights(renderer);

      CameraDPtr camera = rendering->GetTheCamera();
      VisualizationDPtr visualization = rendering->GetTheVisualization();

      Box *gBox = visualization->get_global_box();
      Box *lBox = visualization->get_local_box();

      camera->generate_initial_rays(renderer, renderingSet, rendering, lBox, gBox, rvec, fnum);
    }

#ifdef GXY_PRODUCE_STATUS_MESSAGES
    renderingSet->_dumpState(c, "status"); // Note this will sync after cameras, I think
#endif

#ifdef GXY_EVENT_TRACKING
    GetTheEventTracker()->Add(new CameraLPtroopEndEvent);
#endif

#ifdef GXY_WRITE_IMAGES
    for (auto& r : rvec)
      r.get();

    renderingSet->DecrementActiveCameraCount(0);
#endif // GXY_WRITE_IMAGES
  }
  
}

bool
Renderer::LoadStateFromDocument(Document& doc)
{
  if (doc.HasMember("Renderer"))
    LoadStateFromValue(doc["Renderer"]);

  return true;
}

void
Renderer::SaveStateToDocument(Document& doc)
{
  Value v(kObjectType);
  SaveStateToValue(v, doc);
  doc.AddMember("Renderer", v, doc.GetAllocator());
}


bool
Renderer::LoadStateFromValue(Value& v)
{
  if (v.HasMember("epsilon"))
    SetEpsilon(v["epsilon"].GetDouble());

  return true;
}

void
Renderer::SaveStateToValue(Value& v, Document& doc)
{
  v.AddMember("epsilon", Value().SetDouble(GetEpsilon()), doc.GetAllocator());
}

void
Renderer::Classify(RayList *raylist)
{
  // Tracing the input rays classifies them by an application-specific
  // flag.   Classify classifies these into 4 categories known by the 
  // superclass:  DROP_ON_FLOOR - that is, ignored in further processing;
  // BOUNDARY - that is, hit a boundary without terminating for any 
  // application-specific and thus is a candidate for sending elsewhere 
  // (if its not an external boundary, but thats a question for the
  // partitioning manager); KEEP_HERE - for example, having encountered an
  // application event that caused the ray tracing to break but after which
  // the ray will continue, and TERMINATED, if the ray has reached a
  // terminal state and is ready to update the final result.

  for (int i = 0; i < raylist->GetRayCount(); i++)
  {
    int typ  = raylist->get_type(i);
    int term = raylist->get_term(i);

    if (typ == RAY_PRIMARY)
    {
      raylist->set_classification(i, 0);

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

      if (term & RAY_BOUNDARY)
      {
        raylist->set_classification(i, RAY_BOUNDARY);
      }
      else if ((term & RAY_OPAQUE) | (term & RAY_TIMEOUT))
      {
        raylist->set_classification(i, TERMINATED);
      }
      else		// Translucent surface
      {
        raylist->set_classification(i, KEEP_HERE);
      }
    }
    
    // If its a shadow ray it gets added into the FB if it terminated at the 
    // external boundary.   If it exitted at a internal boundary it keeps 
    // going.   If it hit a surface it dies.

    else if (typ == RAY_SHADOW)
    {
      if ((term & RAY_OPAQUE) | (term & RAY_SURFACE))
      {
        // We don't need to add in the light's contribution
        // OR, if reverse lighting, send to FB to ADD the (negative) shadow

#ifdef GXY_REVERSE_LIGHTING
        raylist->set_classification(i, TERMINATED);
#else
        raylist->set_classification(i, DROP_ON_FLOOR);
#endif
      }
      else if (term & RAY_BOUNDARY)
      {
        raylist->set_classification(i, RAY_BOUNDARY);
      }
      else 
      {
        std::cerr << "Shadow ray died for an unknown reason\n";
        raylist->set_classification(i, DROP_ON_FLOOR);
      }
    }

    // If its an AO ray, same thing - except it CAN time out.
    
    else if (typ == RAY_AO)
    {
      if ((term & RAY_OPAQUE) | (term & RAY_SURFACE))
      {
        // We don't need to add in the light's contribution
        // OR, if reverse lighting, send to FB to ADD the (negative)
        // ambient contribution

#ifdef GXY_REVERSE_LIGHTING
        raylist->set_classification(i, TERMINATED);
#else
        raylist->set_classification(i, DROP_ON_FLOOR);
#endif
      }
      else if (term & RAY_BOUNDARY)
      {
        raylist->set_classification(i, RAY_BOUNDARY);
      }
      else if (term & RAY_TIMEOUT)
      {
        // Timed-out - drop on floor in REVERSE case so
        // we don't add the (negative) ambient contribution;
        // otherwise, send  to FB to add in (positive) ambient
        // contribution
#ifdef GXY_REVERSE_LIGHTING
        raylist->set_classification(i, DROP_ON_FLOOR);
#else
        raylist->set_classification(i, TERMINATED);
#endif
      }
      else
      {
        std::cerr << "AO ray died for an unknown reason\n";
        raylist->set_classification(i, DROP_ON_FLOOR);
      }
    }
  }
}

void
Renderer::AssignDestinations(RayList *raylist)
{
  VisualizationDPtr visualization = raylist->GetTheRendering()->GetTheVisualization();
  Box *box = visualization->get_local_box();

  for (int i = 0; i < raylist->GetRayCount(); i++)
  {
    if (raylist->get_classification(i) == RAY_BOUNDARY)
    {
      int exit_face = box->exit_face(raylist->get_ox(i), raylist->get_oy(i), raylist->get_oz(i),
                                   raylist->get_dx(i), raylist->get_dy(i), raylist->get_dz(i));

      if (visualization->has_neighbor(exit_face)) 
        raylist->set_classification(i, visualization->get_neighbor(exit_face));
      else
      {
        int t = raylist->get_type(i);
        if (t == RAY_SHADOW || t == RAY_AO)
        {
#ifdef GXY_REVERSE_LIGHTING
          raylist->set_classification(i, DROP_ON_FLOOR);
#else
          raylist->set_classification(i, TERMINATED);
#endif
        }
        else
          raylist->set_classification(i, TERMINATED);
      }
    }
  }
}

void
Renderer::HandleTerminatedRays(RayList *raylist)
{
  int terminated_count = 0;

  for (int i = 0; i < raylist->GetRayCount(); i++)
    if (raylist->get_classification(i) == Renderer::TERMINATED) terminated_count++;

  RenderingSetDPtr  renderingSet  = raylist->GetTheRenderingSet();
  RenderingDPtr rendering = raylist->GetTheRendering();

  if (terminated_count == 0) return;

  if (rendering->IsLocal())
  {
    Pixel *local_pixels = new Pixel[terminated_count];

    Pixel *p = local_pixels;
    for (int i = 0; i < raylist->GetRayCount(); i++)
    if (raylist->get_classification(i) == Renderer::TERMINATED)
    {
      p->x = raylist->get_x(i);
      p->y = raylist->get_y(i);
      p->r = raylist->get_r(i);
      p->g = raylist->get_g(i);
      p->b = raylist->get_b(i);
      p->o = raylist->get_o(i);
      p++;
    }

    rendering->AddLocalPixels(local_pixels, terminated_count, raylist->GetFrame(), GetTheApplication()->GetRank());
    delete[] local_pixels;
  }
  else
  {
    Renderer::SendPixelsMsg *spmsg = new Renderer::SendPixelsMsg(rendering, renderingSet, raylist->GetFrame(), terminated_count);

    for (int i = 0; i < raylist->GetRayCount(); i++)
      if (raylist->get_classification(i) == Renderer::TERMINATED)
        spmsg->StashPixel(raylist, i);

    if (renderingSet->IsActive(raylist->GetFrame()))
      spmsg->Send(rendering->GetTheOwner());

    delete spmsg;
  }
}

class processRays_task : public ThreadPoolTask
{
public:
  processRays_task(RayList *raylist, Renderer *renderer) : 
    ThreadPoolTask(raylist->GetType() == RayList::PRIMARY ? 3 : 2), raylist(raylist), renderer(renderer) {}
  ~processRays_task() {}

	int work() { 

		RendererDPtr      renderer      = raylist->GetTheRenderer();
		RenderingSetDPtr  renderingSet  = raylist->GetTheRenderingSet();
		RenderingDPtr     rendering     = raylist->GetTheRendering();
		VisualizationDPtr visualization = rendering->GetTheVisualization();
  
#if 0
    std::cerr << std::hex 
          << "R " << renderer.get() 
          << " r " << rendering.get() 
          << " rs " << renderingSet.get() 
          << " v " << visualization.get()  << "\n";
#endif
    

		// This is called when a list of rays is pulled off the
		// RayQ.  When we are done with it we decrement the 
		// ray list count (rather than when it was pulled off the
		// RayQ) so we don't send a message upstream saying we are idle
		// until we actually are.

    // std::cerr << GetTheApplication()->GetRank() << " received raylist " << raylist->GetRayCount() << "\n";

		while (raylist)
		{
			if (! renderingSet->IsActive(raylist->GetFrame()))
		  {
				// std::cerr << GetTheApplication()->GetRank() << " dropping raylist (" << raylist->GetFrame() << ", " <<  renderingSet->GetCurrentFrame() << ")\n";
				delete raylist;
				return 0;
		  }
			// else
				// std::cerr << GetTheApplication()->GetRank() << " processing raylist " << raylist->GetRayCount() << "\n";

			// This may put secondary lists on the ray queue
			renderer->Trace(raylist);

      // Classify annotated rays
      renderer->Classify(raylist);

      // Assign destinations to rays that need to go elsewhere
      renderer->AssignDestinations(raylist);

      // And handle the ones that terminate
      renderer->HandleTerminatedRays(raylist);

			// OK, now we know the fate of the input rays.   Partition them accordingly
			// counts going to each destination - six exit faces and stay right here.

      int *knts = new int[GetTheApplication()->GetSize()];
      for (int i = 0; i < GetTheApplication()->GetSize(); i++)
        knts[i] = 0;

      int nKeepers = 0;
			for (int i = 0; i < raylist->GetRayCount(); i++)
			{
        int dst = raylist->get_classification(i);

				if (dst == Renderer::KEEP_HERE)
					nKeepers++;
				else if (dst >= 0)
					knts[dst] ++;
				else if (dst != Renderer::DROP_ON_FLOOR && dst != Renderer::TERMINATED)
        {
					std::cerr << "CLASSIFICATION ERROR 1 - " << dst << "\n";
          raylist->print(i);
        }
			}

			// Allocate a RayList for each remote destination's rays and the keepers.
			// Note that // there'll never be more rays being passed to a neighbor from 
			// the current ray list than the number of rays *in* the current ray list, 
			// so we don't have to worry about these being larger than the rays-per-packet
			// limit.
			//
			// They'll bound up in a SendRays message later, without a copy. 7 lists, for
			// each of 6 faces and stay here

			RayList **ray_lists = new RayList*[GetTheApplication()->GetSize()];
      RayList *keepers = (nKeepers > 0) ? new RayList(renderer, renderingSet, rendering, nKeepers, raylist->GetFrame(), raylist->GetType()) : (RayList *)NULL;
        
			for (int i = 0; i < GetTheApplication()->GetSize(); i++)
			{
				if (knts[i])
					ray_lists[i]   = new RayList(renderer, renderingSet, rendering, knts[i], raylist->GetFrame(), raylist->GetType());
				else
					ray_lists[i]   = NULL;

				knts[i] = 0;   // We'll be re-using this to keep track of where the next ray goes in
											 // the output ray list
			}

      nKeepers = 0;
			for (int i = 0; i < raylist->GetRayCount(); i++)
			{
        int clss = raylist->get_classification(i);
				if (clss >= 0)
				{
					if (knts[clss] >= ray_lists[clss]->GetRayCount())
						std::cerr << "CLASSIFICATION ERROR 2\n";

					RayList::CopyRay(raylist, i, ray_lists[clss], knts[clss]);
					knts[clss]++;
				}
        else if (clss == Renderer::KEEP_HERE)
					RayList::CopyRay(raylist, i, keepers, nKeepers++);
      }

			for (int i = 0; i < GetTheApplication()->GetSize(); i++)
				if (knts[i])
				{
#ifdef GXY_WRITE_IMAGES
					// This process gets "ownership" of the new ray list until its recipient acknowleges 
					renderingSet->IncrementRayListCount();
					renderer->SendRays(ray_lists[i], i);
#else
					if (renderingSet->IsActive(ray_lists[i]->GetFrame()))
					{
						renderer->SendRays(ray_lists[i], i);
					}
#endif
          delete ray_lists[i];
        }

      delete raylist;
      delete[] knts;
			delete[] ray_lists;

      // Just loop on the keepers.

			raylist = keepers;
		}

#ifdef GXY_WRITE_IMAGES
    // Finished processing this ray list.  
    renderingSet->DecrementRayListCount();
#endif //  GXY_WRITE_IMAGES

    return 0;
  }

private:
  RayList *raylist;
  Renderer *renderer;
};

void
Renderer::Trace(RayList *raylist)
{
  RendererDPtr      renderer  = raylist->GetTheRenderer();
  RenderingSetDPtr  renderingSet  = raylist->GetTheRenderingSet();
  RenderingDPtr     rendering     = raylist->GetTheRendering();
  VisualizationDPtr visualization = rendering->GetTheVisualization();

  // This is called when a list of rays is pulled off the
  // RayQ.  When we are done with it we decrement the 
  // ray list count (rather than when it was pulled off the
  // RayQ) so we don't send a message upstream saying we are idle
  // until we actually are.

  TraceRays tracer(GetEpsilon());

  RayList *out = tracer.Trace(rendering->GetLighting(), visualization, raylist);
  if (out)
  {
    if (out->GetRayCount() > renderer->GetMaxRayListSize())
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
  RendererDPtr r = GetByKey(key);
  r->_dumpStats();
  return false;
}

void 
Renderer::SendRays(RayList *rays, int destination)
{
#ifdef GXY_WRITE_IMAGES
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
  rayList->GetTheRenderer()->_received_from(sender, nReceived);

  RenderingDPtr rendering = rayList->GetTheRendering();
  RenderingSetDPtr renderingSet = rendering ? rayList->GetTheRenderingSet() : NULL;
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

#ifdef GXY_WRITE_IMAGES
  Renderer::AckRaysMsg ack(renderingSet);
  ack.Send(sender);
#endif // GXY_WRITE_IMAGES
  
  return false;
}

int
Renderer::SerialSize()
{
  return sizeof(bool) + sizeof(int);
}

unsigned char *
Renderer::Serialize(unsigned char *p)
{
  *(bool*)p = permute_pixels;
  p += sizeof(bool);
  *(int*)p = max_rays_per_packet;
  p += sizeof(int);

  return p;
}

unsigned char *
Renderer::Deserialize(unsigned char *p)
{
  permute_pixels = *(bool*)p;
  p += sizeof(bool);
  max_rays_per_packet = *(int*)p;
  p += sizeof(int);

  return p;
}

#ifdef GXY_WRITE_IMAGES
Renderer::AckRaysMsg::AckRaysMsg(RenderingSetDPtr rs) : AckRaysMsg(sizeof(Key))
{
  *(Key *)contents->get() = rs->getkey();
}

bool
Renderer::AckRaysMsg::Action(int sender)
{
  RenderingSetDPtr rs = RenderingSet::GetByKey(*(Key *)contents->get());
  rs->DecrementInFlightCount();
  rs->DecrementRayListCount();
  return false;
}
#endif // GXY_WRITE_IMAGES

void
Renderer::Start(RenderingSetDPtr rs)
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
  msg.Broadcast(false, true);
}

void Renderer::DumpStatistics()
{
  StatisticsMsg *s = new StatisticsMsg(this);
  s->Broadcast(true, true);
}

Renderer::RenderMsg::~RenderMsg()
{
}

Renderer::RenderMsg::RenderMsg(Renderer* r, RenderingSetDPtr rs) :
  Renderer::RenderMsg(sizeof(Key) + r->SerialSize() + sizeof(Key))
{
  unsigned char *p = contents->get();
  *(Key *)p = r->getkey();
  p = p + sizeof(Key);
  p = r->Serialize(p);
  *(Key *)p = rs->getkey();
}

bool
Renderer::RenderMsg::Action(int sender)
{
  unsigned char *p = (unsigned char *)get();
  Key renderer_key = *(Key *)p;
  p += sizeof(Key);

  RendererDPtr renderer = Renderer::GetByKey(renderer_key);
  p = renderer->Deserialize(p);

  RenderingSetDPtr rs = RenderingSet::GetByKey(*(Key *)p);

  renderer->local_render(renderer, rs);

  return false;
}

} // namespace gxy
