#include "vector"
#include "map"
#include "vector"
#include "memory"
#include "gxy/KeyedDataObject.h"
#include "gxy/Volume.h"

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

class RungeKutta: public KeyedDataObject
{
  KEYED_OBJECT_SUBCLASS(RungeKutta, KeyedDataObject)

public:
  static void RegisterRK()
  {
    RungeKutta::RegisterClass();
    RungeKutta::RKTraceMsg::Register();
  }

  virtual void initialize();

  void Trace(int id, vec3f& pt, VolumeP v);
  void _Trace(int where, int id, int n, vec3f& pt, vec3f& up, float time, VolumeP v);

  virtual void local_trace(int id, int n, vec3f& pt, vec3f& up, float time, VolumeP v);

  int  get_max_steps() { return max_steps; }
  void set_max_steps(int n) { max_steps = n; }

  int get_number_of_local_trajectories() { return trajectories.size(); }

  void get_keys(std::vector<int>& v)
  {
    v.clear();
    for (auto it = trajectories.begin(); it != trajectories.end(); it++)
      v.push_back(it->first);
  }

  trajectory get_trajectory(int id) { return trajectories[id]; }
    

protected:
  pthread_mutex_t lock;

  virtual int serialSize();
  virtual unsigned char* serialize(unsigned char *ptr);
  virtual unsigned char* deserialize(unsigned char *ptr);

  std::map<int, trajectory> trajectories;

  int max_steps;

  class RKTraceMsg : public Work
  {
  public:
    RKTraceMsg(Key rkk, VolumeP vp, int id, int n, float t, vec3f u, vec3f p) :
       RKTraceMsg(2*sizeof(Key) + 2*sizeof(int) + sizeof(float) + 2*sizeof(vec3f)) 
    {
      unsigned char *g = (unsigned char *)get();
      *(Key *)g = rkk;
      g += sizeof(Key);
      *(Key *)g = vp->getkey();
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
      VolumeP vp = Volume::GetByKey(*(Key *)g);
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

      rkp->local_trace(id, n, p, u, t, vp);

      return false;
    }
  };
};

}
