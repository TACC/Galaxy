#include "RungeKutta.h"
#include "gxy/Application.h"
#include "gxy/Volume.h"

using namespace std;

namespace gxy
{

WORK_CLASS_TYPE(RungeKutta::RKTraceMsg)
KEYED_OBJECT_CLASS_TYPE(RungeKutta)

void
RungeKutta::initialize()
{
  super::initialize();
  max_steps = 1000;
}


int 
RungeKutta::serialSize() { return super::serialSize() + sizeof(int); }

unsigned char *
RungeKutta::serialize(unsigned char *ptr)
{
  ptr = super::serialize(ptr);
  *(int *)ptr = max_steps; ptr += sizeof(int);
  return ptr;
}

unsigned char *
RungeKutta::deserialize(unsigned char *ptr)
{
  ptr = super::deserialize(ptr);
  max_steps = *(int *)ptr; ptr += sizeof(int);
  return ptr;
}

void
RungeKutta::Trace(int id, vec3f& p, VolumeP v)
{
  vec3f u(0.0, 1.0, 0.0);
  _Trace(id, 0, p, u, 0.0, v);
}

void
RungeKutta::_Trace(int id, int n, vec3f& p, vec3f& u, float t, VolumeP vp)
{
  int whose = vp->PointOwner(p);
  if (whose != -1)
  {
    RKTraceMsg msg(getkey(), vp, id, n, t, u, p);
    msg.Send(whose);
  }
  else
    std::cerr << "RungeKutta::Trace: p is outside volume\n";
}

void
RungeKutta::local_trace(int id, int n, vec3f& p, vec3f& u, float t, VolumeP v)
{
  trajectory traj;
  vec3f pLast, uLast;
  float tLast;
  int me = GetTheApplication()->GetRank();

  // h will be the step size

  vec3f d;
  v->get_deltas(d.x, d.y, d.z);

  float h = d.x > d.y ? d.y > d.z ? d.z : d.y : d.x > d.z ? d.z : d.x;

  bool terminated = false;
  while (! terminated)
  {
    float vel[3];
    v->Sample(p, vel);

    vec3f velocity(vel[0], vel[1], vel[2]);

    float l = len(velocity);
    if (l < 0.001)
    {
      terminated = true;
      zero(velocity);
    }
    else
      normalize(velocity);

    // std::cerr << "(" << me << ") p (" << p.x << "," << p.y << "," << p.z << ") velocity (" << velocity.x << "," << velocity.y << "," << velocity.z << ") t " << t << "\n";
    std::cerr << me << "," << p.x << "," << p.y << "," << p.z << "," << velocity.x << "," << velocity.y << "," << velocity.z << "," << t << "\n";

    traj.points.push_back(p);
    traj.tangents.push_back(velocity);
    traj.ups.push_back(u);
    traj.times.push_back(t);

    if (n + traj.times.size() > max_steps)
      terminated = true;

    uLast = u;
    pLast = p;
    tLast = t;

    if (v->PointOwner(p) != me)
      break;

    vec3f k1 = velocity * h;

    vec3f p2 = p + (k1 * 0.5), v2;
    v->Sample(p2, v2);
    vec3f k2 = v2 * h;

    vec3f  p3 = p + (k2 * 0.5), v3;
    v->Sample(p3, v3);
    vec3f k3 = v3 * h;

    vec3f  p4 = p + k3, v4;
    v->Sample(p4, v4);
    vec3f k4 = v4 * h;

    p = p + (k1 + (k2 * 2) + (k3 * 2) + k4) * (1.0/6.0);
    t = t + h;
  }

  std::map<int, vector<trajectory>>::iterator ti = traces.find(id);
  if (ti == traces.end())
  {
    vector<trajectory> vtraj;
    vtraj.push_back(traj);
    traces[id] = vtraj;
  }
  else
    ti->second.push_back(traj);

  if (! terminated)
    _Trace(id, n + traj.points.size(), p, u, t, v);
}

}

