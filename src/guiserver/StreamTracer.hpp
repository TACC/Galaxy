// ========================================================================== //
//                                                                            //
// Copyright (c) 2014-2019 The University of Texas at Austin.                 //
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

#include "vector"
#include "map"
#include "vector"
#include "memory"
#include "KeyedDataObject.h"
#include "Volume.h"
#include "Particles.h"
#include "PathLines.h"
#include "Filter.h"

namespace gxy
{

OBJECT_POINTER_TYPES(StreamTracer)

struct _segment
{
  _segment() {}
  ~_segment() {}

  std::vector<vec3f> points;
  std::vector<vec3f> ups;
  std::vector<vec3f> tangents;
  std::vector<float> times;
};

typedef std::shared_ptr<_segment> segment;
typedef std::shared_ptr<std::vector<segment>> trajectory;

#define STREAMTRACER_INFLIGHT_OFFSET  999999999

class StreamTracer: public KeyedDataObject 
{
  KEYED_OBJECT_SUBCLASS(StreamTracer, KeyedDataObject)

public:
  virtual void initialize();

  void Trace(vec3f& pt, int id = 0);
  void Trace(int n, vec3f* pts);
  void _Trace(int where, int id, int n, vec3f& pt, vec3f& up, float time);
  void Trace(ParticlesP pp);

  void TraceToPathLines(PathLinesP plp);
  
  virtual void local_trace(int id, int n, vec3f& pt, vec3f& up, float time);

  int get_number_of_local_trajectories() { return trajectories.size(); }
  float get_maximum_integration_time() { return max_integration_time; }

  void get_keys(std::vector<int>& v)
  {
    v.clear();
    for (auto it = trajectories.begin(); it != trajectories.end(); it++)
      v.push_back(it->first);
  }

  trajectory get_trajectory(int id) { return trajectories[id]; }

  virtual bool local_commit(MPI_Comm);

  void decrement_in_flight(float t)
  {
    Lock();
    // if (t > max_integration_time) max_integration_time = t;
    in_flight --;
    if (in_flight == 0)
      Signal();
    Unlock();
  }

  void remove_offset(int n)
  {
    Lock();
    in_flight -= (STREAMTRACER_INFLIGHT_OFFSET - n);
    if (in_flight == 0)
      Signal(); 
    Unlock();
  }

  void Lock() { pthread_mutex_lock(&lock); }
  void Unlock() { pthread_mutex_unlock(&lock); }
  void Signal() { pthread_cond_signal(&signal); }
  void Wait() { pthread_cond_wait(&signal, &lock); }

  void SetVectorField(VolumeP v) { vectorField = v; }
  VolumeP GetVectorField() { return vectorField; }

  int  get_max_steps() { return max_steps; }
  void set_max_steps(int n) { max_steps = n; }

  float get_stepsize() { return stepsize; }
  void set_stepsize(float s) { stepsize = s; }

  float GetMinVelocity() { return min_velocity; }
  void SetMinVelocity(float v) { min_velocity = v; }

  float GetMaxIntegrationTime() { return max_integration_time; }
  void SetMaxIntegrationTime(float t) { max_integration_time = t; }

  float GetTStart() { return tStart; }
  void SetTStart(float t) { tStart = t; }

  float GetDeltaT() { return deltaT; }
  void SetDeltaT(float t) { deltaT = t; }

protected:
  VolumeP vectorField;

  pthread_mutex_t lock;
  pthread_cond_t signal;

  int in_flight;

  virtual int serialSize();
  virtual unsigned char* serialize(unsigned char *ptr);
  virtual unsigned char* deserialize(unsigned char *ptr);

  std::map<int, trajectory> trajectories;

  int   max_steps            = 1000;
  float stepsize             = 0.2;
  float min_velocity         = -1;
  float max_integration_time = -1;
  float tStart               = -1;
  float deltaT               = -1;

  class StreamTracerMsg : public Work
  {
  public:
    StreamTracerMsg(Key rkk, int id, int n, float t, vec3f u, vec3f p) :
       StreamTracerMsg(sizeof(Key) + 2*sizeof(int) + sizeof(float) + 2*sizeof(vec3f)) 
    {
      unsigned char *g = (unsigned char *)get();
      *(Key *)g = rkk;
      g += sizeof(Key);
      *(int *)g = n;
      g += sizeof(int);
      *(int *)g = id;
      g += sizeof(int);
      *(float *)g = t;
      g += sizeof(float);
      memcpy(g, &p, sizeof(vec3f));
      g += sizeof(vec3f);
      memcpy(g, &u, sizeof(vec3f));
      g += sizeof(vec3f);
    }

    ~StreamTracerMsg() {}
    WORK_CLASS(StreamTracerMsg, true)

  public:
    bool Action(int s)
    {
      unsigned char *g = (unsigned char *)get();
      StreamTracerP stp = StreamTracer::GetByKey(*(Key *)g);
      g += sizeof(Key);
      int n = *(int *)g;
      g += sizeof(int);
      int id = *(int *)g;
      g += sizeof(int);
      float t = *(float *)g;
      g += sizeof(float);
      vec3f p, u;
      memcpy(&p, g, sizeof(vec3f));
      g += sizeof(vec3f);
      memcpy(&u, g, sizeof(vec3f));

      stp->local_trace(id, n, p, u, t);

      return false;
    }
  };

  class StreamTracerCompleteMsg : public Work
  {
  public:
    StreamTracerCompleteMsg(Key rkk, float t) : StreamTracerCompleteMsg(sizeof(Key) + sizeof(float))
    {
      unsigned char *g = (unsigned char *)get();
      *(Key *)g = rkk;
      g += sizeof(Key);
      *(float *)g = t;
      g += sizeof(float);
    }

    ~StreamTracerCompleteMsg() {}
    WORK_CLASS(StreamTracerCompleteMsg, true)

  public:
    bool Action(int s)
    {
      unsigned char *g = (unsigned char *)get();
      StreamTracerP stp = StreamTracer::GetByKey(*(Key *)g);
      g += sizeof(Key);
      float t = *(float *)g;

      stp->decrement_in_flight(t);

      return false;
    }
  };

  class StreamTracerFromParticleSetMsg : public Work
  {
  public:
    StreamTracerFromParticleSetMsg(Key rkk, ParticlesP pp, int n) :
       StreamTracerFromParticleSetMsg(2*sizeof(Key) + sizeof(int))
    {
      unsigned char *g = (unsigned char *)get();
      *(Key *)g = rkk;
      g += sizeof(Key);
      *(Key *)g = pp->getkey();
      g += sizeof(Key);
      *(int *)g = n;
      g += sizeof(int);
    }

    ~StreamTracerFromParticleSetMsg() {}

    WORK_CLASS(StreamTracerFromParticleSetMsg, true)

  class trace_task : public ThreadPoolTask
  {
  public: 
    trace_task(StreamTracerP _stp, vec3f _p, int _id) : 
      ThreadPoolTask(3), stp(_stp), p(_p), id(_id) {}

    int work()
    {
      vec3f u(0.0, 0.0, 0.0);
      stp->local_trace(id, 0, p, u, 0.0);
      return 0;
    }
  private:
    StreamTracerP stp;
    vec3f p;
    int id;
  };
  
  public:
    bool Action(int s)
    {
      unsigned char *g = (unsigned char *)get();
      Key rkk = *(Key *)g;
      g += sizeof(Key);
      Key pk  = *(Key *)g;
      g += sizeof(Key);
      int n = *(int *)g;
      g += sizeof(int);

      StreamTracerP stp = StreamTracer::GetByKey(rkk);
      ParticlesP pp = Particles::GetByKey(pk);
      
      int rank = GetTheApplication()->GetRank();
      int size = GetTheApplication()->GetSize();

      vec3f *vertices = pp->GetVertices();
      int nTraces = 0;
      while (nTraces < pp->GetNumberOfVertices())
      {
        trace_task *tt = new trace_task(stp, vertices[nTraces], n+nTraces);
        GetTheApplication()->GetTheThreadPool()->AddTask(tt);
        nTraces++;
      }

      if (size == 1)
      {
        // Then we've kicked off all the traces we are going to.  Remove offset and 
        // set the inflight count to reflect the number of traces we've spawned.
        // If any traces complete *before we get here* this will ensure
        // that their completion will be taken into account

        stp->remove_offset(nTraces);
      }
      else if (rank == (size - 1))
      {
        // If so, then we need to inform the master of the number that were spawned.

        StreamTracerCountMsg msg(rkk, n + pp->GetNumberOfVertices());
        msg.Send(0);
      }
      else
      {
        // Otherwise, tell the next process to start its streamlines

        StreamTracerFromParticleSetMsg msg(rkk, pp, n + pp->GetNumberOfVertices());
        msg.Send(rank+1);
      }


      return false;
    }
  };

  class StreamTracerCountMsg : public Work
  {
  public:
    StreamTracerCountMsg(Key rkk, int n) : StreamTracerCountMsg(sizeof(Key) + sizeof(int))
    {
      unsigned char *g = (unsigned char *)get();
      *(Key *)g = rkk;
      g += sizeof(Key);
      *(int *)g = n;
      g += sizeof(int);
    }

    ~StreamTracerCountMsg() {}
    WORK_CLASS(StreamTracerCountMsg, true)

  public:
    bool Action(int s)
    {
      unsigned char *g = (unsigned char *)get();
      StreamTracerP stp = StreamTracer::GetByKey(*(Key *)g);
      g += sizeof(Key);
      int n = *(int *)g;

      stp->remove_offset(n);

      return false;
    }
  };

  class StreamTracerTraceToPathLinesMsg : public Work
  {
  public:
    StreamTracerTraceToPathLinesMsg(StreamTracer* st, PathLinesP plp)
      : StreamTracerTraceToPathLinesMsg(2*sizeof(Key))
    {
      unsigned char *g = (unsigned char *)get();
      *(Key *)g = st->getkey(); g += sizeof(Key);
      *(Key *)g = plp->getkey(); g += sizeof(Key);
    }

    ~StreamTracerTraceToPathLinesMsg() {}

    WORK_CLASS(StreamTracerTraceToPathLinesMsg, true)

  public:
    bool CollectiveAction(MPI_Comm c, bool is_root)
    {
      unsigned char *g = (unsigned char *)get();
      StreamTracerP stp = StreamTracer::GetByKey(*(Key *)g); g += sizeof(Key);
      PathLinesP plp = PathLines::GetByKey(*(Key *)g); g += sizeof(Key);

      plp->CopyPartitioning(stp->GetVectorField());

      float t = stp->GetTStart();
      float dt = stp->GetDeltaT();

      std::vector<int> keys;
      stp->get_keys(keys);

      plp->clear();

      // How much space do I need?

      int np = 0, nc = 0, ii = 0;
      for (auto id = keys.begin(); id != keys.end(); id++, ii++)
      {
        trajectory traj = stp->get_trajectory(*id);
        for (auto segp = traj->begin(); segp != traj->end(); segp++)
        {
          auto seg = *segp;

          auto k = 0;
          for (auto n = 0; n < seg->times.size(); n++)
            if (t == -1 || (seg->times[n] > (t - dt) && seg->times[n] < t))
              k++;

            if (k > 1)
            {
              np += k;
              nc += k - 1;
            }
        }
      }

      plp->allocate(np, nc);
      
      vec3f *pbuf = plp->GetVertices();
      float *dbuf = plp->GetData();
      int   *cbuf = plp->GetConnectivity();
  
      np = 0; nc = 0;
      for (auto id = keys.begin(); id != keys.end(); id++)
      {
        trajectory traj = stp->get_trajectory(*id);
        for (auto segp = traj->begin(); segp != traj->end(); segp++)
        {
          auto seg = *segp;
          auto k = 0, ik = -1;
          for (auto n = 0; n < seg->times.size(); n++)
            if (t == -1 || (seg->times[n] > (t - dt) && seg->times[n] < t))
            {
              k++;
              if (ik < 0) ik = n;
            }
  
          if (k > 1)
          {
            memcpy((void *)(pbuf + np), ((vec3f *)seg->points.data()) + ik, k*sizeof(vec3f));
            memcpy((void *)(dbuf + np), ((float *)seg->times.data()) + ik,  k*sizeof(float));
            for (int i = 0; i < k; i++, np++)
              if (i < (k - 1))
                cbuf[nc++] = np;
          }
        }
      }
      MPI_Barrier(c);
  
      return false;
    }
  };
};

class StreamTracerFilter : public Filter
{
  static int stfilter_index;
public:

  StreamTracerFilter() 
  {
    std::stringstream ss;
    ss << "StreamTracerFilter_" << stfilter_index++;
    name = ss.str();

    streamTracer = StreamTracer::NewP();
    result = KeyedDataObject::Cast(PathLines::NewP());
  }

  bool SetVectorField(VolumeP v);
  VolumeP GetVectorField() { return vectorField; }

  int  get_max_steps() { return max_steps; }
  void set_max_steps(int n) { max_steps = n; }

  float get_stepsize() { return stepsize; }
  void set_stepsize(float s) { stepsize = s; }

  float GetMinVelocity() { return min_velocity; }
  void SetMinVelocity(float v) { min_velocity = v; }

  float GetMaxIntegrationTime() { return max_integration_time; }
  void SetMaxIntegrationTime(float t) { max_integration_time = t; }

  float GetTStart() { return tStart; }
  void SetTStart(float t) { tStart = t; }

  float GetDeltaT() { return deltaT; }
  void SetDeltaT(float t) { deltaT = t; }

  StreamTracerP GetTheStreamlines() { return streamTracer; }

  void Trace(ParticlesP seeds)
  {
    streamTracer->SetVectorField(vectorField);
    streamTracer->set_max_steps(max_steps);
    streamTracer->set_stepsize(stepsize);
    streamTracer->SetMinVelocity(min_velocity);
    streamTracer->SetMaxIntegrationTime(max_integration_time);
    streamTracer->Commit();
    streamTracer->Trace(seeds);
  }
  
  void TraceToPathLines()
  {
    streamTracer->SetTStart(tStart);
    streamTracer->SetDeltaT(deltaT);
    streamTracer->Commit();

    PathLinesP plp = PathLines::Cast(result);
    streamTracer->TraceToPathLines(plp);
  }
  
private:
  StreamTracerP streamTracer;

  VolumeP vectorField;

  int   max_steps            = 1000;
  float stepsize             = 0.2;
  float min_velocity         = -1;
  float max_integration_time = -1;
  float tStart               = -1;
  float deltaT               = -1;

};

}
