#include "vector"
#include "map"
#include "vector"
#include "memory"
#include "KeyedDataObject.h"
#include "Threading.h"
#include "Volume.h"
#include "Particles.h"

namespace gxy
{

OBJECT_POINTER_TYPES(RungeKutta)

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

#define RUNGEKUTTA_INFLIGHT_OFFSET  999999999

class RungeKutta: public KeyedDataObject
{
  KEYED_OBJECT_SUBCLASS(RungeKutta, KeyedDataObject)

public:
  static void RegisterRK()
  {
    RungeKutta::RegisterClass();
    RungeKutta::RKTraceMsg::Register();
    RungeKutta::RKTraceCompleteMsg::Register();
    RungeKutta::RKTraceCountMsg::Register();
    RungeKutta::RKTraceFromParticleSetMsg::Register();
  }

  virtual void initialize();

  void Trace(vec3f& pt, int id = 0);
  void Trace(int n, vec3f* pts);
  void _Trace(int where, int id, int n, vec3f& pt, vec3f& up, float time);
  void Trace(ParticlesP pp);
  
  virtual void local_trace(int id, int n, vec3f& pt, vec3f& up, float time);

  int  get_max_steps() { return max_steps; }
  void set_max_steps(int n) { max_steps = n; }

  float get_stepsize() { return stepsize; }
  void set_stepsize(float s) { stepsize = s; }

  int get_number_of_local_trajectories() { return trajectories.size(); }
  float get_maximum_integration_time() { return max_integration_time; }

  void get_keys(std::vector<int>& v)
  {
    v.clear();
    for (auto it = trajectories.begin(); it != trajectories.end(); it++)
      v.push_back(it->first);
  }

  trajectory get_trajectory(int id) { return trajectories[id]; }

  bool SetVectorField(VolumeP v);
  VolumeP GetVectorField() { return vectorField; }

  virtual bool local_commit(MPI_Comm);

  void decrement_in_flight(float t)
  {
    Lock();
    if (t > max_integration_time) max_integration_time = t;
    in_flight --;
    if (in_flight == 0)
      Signal();
    Unlock();
  }

  void remove_offset(int n)
  {
    Lock();
    in_flight -= (RUNGEKUTTA_INFLIGHT_OFFSET - n);
    if (in_flight == 0)
      Signal(); 
    Unlock();
  }

  void Lock() { pthread_mutex_lock(&lock); }
  void Unlock() { pthread_mutex_unlock(&lock); }
  void Signal() { pthread_cond_signal(&signal); }
  void Wait() { pthread_cond_wait(&signal, &lock); }

  float GetMinVelocity() { return min_velocity; }
  void SetMinVelocity(float v) { min_velocity = v; }

  float GetMaxIntegrationTime() { return max_integration_time; }
  void SetMaxIntegrationTime(float t) { max_integration_time = t; }

protected:
  VolumeP vectorField;

  pthread_mutex_t lock;
  pthread_cond_t signal;

  int in_flight;
  float max_integration_time;

  virtual int serialSize();
  virtual unsigned char* serialize(unsigned char *ptr);
  virtual unsigned char* deserialize(unsigned char *ptr);

  std::map<int, trajectory> trajectories;

  int max_steps;
  float stepsize;
  float min_velocity;
  float max_time;

  class RKTraceMsg : public Work
  {
  public:
    RKTraceMsg(Key rkk, int id, int n, float t, vec3f u, vec3f p) :
       RKTraceMsg(sizeof(Key) + 2*sizeof(int) + sizeof(float) + 2*sizeof(vec3f)) 
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

    ~RKTraceMsg() {}
    WORK_CLASS(RKTraceMsg, true)

  public:
    bool Action(int s)
    {
      unsigned char *g = (unsigned char *)get();
      RungeKuttaP rkp = RungeKutta::GetByKey(*(Key *)g);
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

      rkp->local_trace(id, n, p, u, t);

      return false;
    }
  };

  class RKTraceCompleteMsg : public Work
  {
  public:
    RKTraceCompleteMsg(Key rkk, float t) : RKTraceCompleteMsg(sizeof(Key) + sizeof(float))
    {
      unsigned char *g = (unsigned char *)get();
      *(Key *)g = rkk;
      g += sizeof(Key);
      *(float *)g = t;
      g += sizeof(float);
    }

    ~RKTraceCompleteMsg() {}
    WORK_CLASS(RKTraceCompleteMsg, true)

  public:
    bool Action(int s)
    {
      unsigned char *g = (unsigned char *)get();
      RungeKuttaP rkp = RungeKutta::GetByKey(*(Key *)g);
      g += sizeof(Key);
      float t = *(float *)g;

      rkp->decrement_in_flight(t);

      return false;
    }
  };

  class RKTraceFromParticleSetMsg : public Work
  {
  public:
    RKTraceFromParticleSetMsg(Key rkk, ParticlesP pp, int n) :
       RKTraceFromParticleSetMsg(2*sizeof(Key) + sizeof(int))
    {
      unsigned char *g = (unsigned char *)get();
      *(Key *)g = rkk;
      g += sizeof(Key);
      *(Key *)g = pp->getkey();
      g += sizeof(Key);
      *(int *)g = n;
      g += sizeof(int);
    }

    ~RKTraceFromParticleSetMsg() {}

    WORK_CLASS(RKTraceFromParticleSetMsg, true)

  class trace_task : public ThreadPoolTask
  {
  public: 
    trace_task(RungeKuttaP _rkp, vec3f _p, int _id) : 
      ThreadPoolTask(3), rkp(_rkp), p(_p), id(_id) {}

    int work()
    {
      vec3f u(0.0, 0.0, 0.0);
      rkp->local_trace(id, 0, p, u, 0.0);
      return 0;
    }
  private:
    RungeKuttaP rkp;
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

      RungeKuttaP rkp = RungeKutta::GetByKey(rkk);
      ParticlesP pp = Particles::GetByKey(pk);
      
      int rank = GetTheApplication()->GetRank();
      int size = GetTheApplication()->GetSize();

      vec3f *vertices = pp->GetVertices();
      for (int i = 0; i < pp->GetNumberOfVertices(); i++)
      {
        trace_task *tt = new trace_task(rkp, vertices[i], n+i);
        GetTheApplication()->GetTheThreadPool()->AddTask(tt);
      }

      if (size == 1)
      {
        // Then we've kicked off all the traces we are going to.  Remove offset and 
        // set the inflight count to reflect the number of traces we've spawned.
        // If any traces complete *before we get here* this will ensure
        // that their completion will be taken into account

        rkp->remove_offset(pp->GetNumberOfVertices());
      }
      else if (rank == (size - 1))
      {
        // If so, then we need to inform the master of the number that were spawned.

        RKTraceCountMsg msg(rkk, n + pp->GetNumberOfVertices());
        msg.Send(0);
      }
      else
      {
        // Otherwise, tell the next process to start its streamlines

        RKTraceFromParticleSetMsg msg(rkk, pp, n + pp->GetNumberOfVertices());
        msg.Send(rank+1);
      }


      return false;
    }
  };

  class RKTraceCountMsg : public Work
  {
  public:
    RKTraceCountMsg(Key rkk, int n) : RKTraceCountMsg(sizeof(Key) + sizeof(int))
    {
      unsigned char *g = (unsigned char *)get();
      *(Key *)g = rkk;
      g += sizeof(Key);
      *(int *)g = n;
      g += sizeof(int);
    }

    ~RKTraceCountMsg() {}
    WORK_CLASS(RKTraceCountMsg, true)

  public:
    bool Action(int s)
    {
      unsigned char *g = (unsigned char *)get();
      RungeKuttaP rkp = RungeKutta::GetByKey(*(Key *)g);
      g += sizeof(Key);
      int n = *(int *)g;

      rkp->remove_offset(n);

      return false;
    }
  };

};

}
