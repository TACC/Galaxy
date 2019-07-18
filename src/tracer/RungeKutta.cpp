#include "RungeKutta.h"
#include "gxy/Application.h"
#include "gxy/Volume.h"

#include <pthread.h>

using namespace std;

namespace gxy
{

WORK_CLASS_TYPE(RungeKutta::RKTraceMsg)
WORK_CLASS_TYPE(RungeKutta::RKTraceCompleteMsg)
WORK_CLASS_TYPE(RungeKutta::RKTraceCountMsg)
WORK_CLASS_TYPE(RungeKutta::RKTraceFromParticleSetMsg)
KEYED_OBJECT_CLASS_TYPE(RungeKutta)

void
RungeKutta::initialize()
{
  super::initialize();
  max_steps = 1000;
  pthread_cond_init(&signal, NULL);
  pthread_mutex_init(&lock, NULL);
}

int 
RungeKutta::serialSize() { return super::serialSize() + sizeof(Key) + sizeof(int); }

bool
RungeKutta::SetVectorField(VolumeP v)
{
  if (!v || v->get_number_of_components() != 3 || v->get_type() != Volume::FLOAT)
  {
    std::cerr << "RungeKutta::SetVectorField:  bad vector field\n";
    return false;
  }
  vectorField = v;
  return true;
}

unsigned char *
RungeKutta::serialize(unsigned char *ptr)
{
  ptr = super::serialize(ptr);
  *(Key *)ptr = vectorField->getkey(); ptr += sizeof(Key);
  *(int *)ptr = max_steps; ptr += sizeof(int);
  return ptr;
}

unsigned char *
RungeKutta::deserialize(unsigned char *ptr)
{
  ptr = super::deserialize(ptr);
  vectorField = Volume::GetByKey(*(Key *)ptr); ptr += sizeof(Key);
  max_steps = *(int *)ptr; ptr += sizeof(int);
  return ptr;
}

void
RungeKutta::Trace(vec3f& p, int id)
{
  Lock();

  in_flight = 1;
  
  vec3f u(0.0, 1.0, 0.0);
  _Trace(GetVectorField()->PointOwner(p), id, 0, p, u, 0.0);

  while (in_flight) 
    Wait();

  Unlock();
}

void
RungeKutta::Trace(int n, vec3f* p)
{
  Lock();

  in_flight = n;

  vec3f u(0.0, 1.0, 0.0);
  
  for (int i = 0; i < n; i++)
    _Trace(GetVectorField()->PointOwner(p[i]), i, 0, p[i], u, 0.0);

  while (in_flight) 
    Wait();

  Unlock();
}

void
RungeKutta::Trace(ParticlesP p)
{
  Lock();

  // Offset by a huge number so that the completion of 
  // traces before the actual number is known don't
  // make it think its done.   The message from the 
  // last process will remove this offset. 

  in_flight = RUNGEKUTTA_INFLIGHT_OFFSET;

  RKTraceFromParticleSetMsg msg(getkey(), p, 0);
  msg.Send(0);

  while (in_flight) 
    Wait();

  Unlock();
}

void
RungeKutta::_Trace(int where, int id, int n, vec3f& p, vec3f& u, float t)
{
  if (where != -1)
  {
    RKTraceMsg msg(getkey(), id, n, t, u, p);
    msg.Send(where);
  }
}

bool
RungeKutta::local_commit(MPI_Comm c)
{
  super::local_commit(c);
  CopyPartitioning(vectorField);
  return false;
}

void
RungeKutta::local_trace(int id, int n, vec3f& p, vec3f& u, float t)
{
  segment seg(new _segment);
  vec3f pLast, uLast;
  float tLast;
  int me = GetTheApplication()->GetRank();

  VolumeP v = GetVectorField();

  // h will be the step size

  vec3f d;
  v->get_deltas(d.x, d.y, d.z);

  float h = d.x > d.y ? d.y > d.z ? d.z : d.y : d.x > d.z ? d.z : d.x;

  // If this is the first point, find a reasonable up

  if (n == 0)
  {
    // If no velocity, no up
    
    vec3f velocity;
    v->Sample(p, velocity);

    float l = len(velocity);
    if (l < 0.001)
      zero(u);
    else
    {
      normalize(velocity);
      vec3f t(1.0, 0.0, 0.0);
      if ((t * velocity) == 0)
        t = vec3f(0.0, 1.0, 0.0);
      cross(velocity, t, u);
    }
  }

  pLast = p;
  uLast = u;
  tLast = t;
  
  int next = me;

  bool terminated = false; vec3f last_tangent;
  while (! terminated)
  {
    float vel[3];
    v->Sample(p, vel);

    vec3f velocity(vel[0], vel[1], vel[2]), normalized_velocity;

    float l = len(velocity);
    if (l < 0.001)
    {
      terminated = true;
      zero(velocity);
      zero(normalized_velocity);
    }
    else
      normalized_velocity = velocity * (1.0 / l);

    // If this is the first point, get an initial up as anything perpendicular to the 
    // initial velocity vector.   Otherwise, rotate the last one.

    float twist = 0;


#if 0
    std::cerr << me << "," 
              << p.x << "," << p.y << "," << p.z << "," 
              << velocity.x << "," << velocity.y << "," << velocity.z << "," 
              << u.x << "," << u.y << "," << u.z << "," 
              << t << "," << twist << "\n";
#endif

    seg->points.push_back(p);
    seg->tangents.push_back(velocity);
    seg->ups.push_back(u);
    seg->times.push_back(t);

    n ++;
    if (n > max_steps)
    {
      terminated = true;
      break;
    }

    if (len(velocity) == 0.0)
      break;

    last_tangent = normalized_velocity;

    // Now take a Runge Kutta step...

    vec3f k1 = normalized_velocity * h;

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

    // Now rotate the up vector.  first get directional derivatives
    // using central difference if both ends are interpolatable

    vec3f s, step, v0, v1;
    vec3f dX, dY, dZ;

    step = vec3f(h, 0, 0);
    s = p - step;
    if (v->Sample(s, v0))
    {
      s = p + step;
      if (v->Sample(s, v1))
        dX = (v1 - v0) * (1.0 / (2.0 * h));
      else
        dX = (velocity - v0) * (1.0 / h);
    }
    else
    {
      s = p + step;
      v->Sample(s, v1);
      dX = (v1 - velocity) * (1.0 / h);
    }

    step = vec3f(0, h, 0);
    s = p - step;
    if (v->Sample(s, v0))
    {
      s = p + step;
      if (v->Sample(s, v1))
        dY = (v1 - v0) * (1.0 / (2.0 * h));
      else
        dY = (velocity - v0) * (1.0 / h);
    }
    else
    {
      s = p + step;
      v->Sample(s, v1);
      dZ = (v1 - velocity) * (1.0 / h);
    }

    step = vec3f(0, 0, h);
    s = p - step;
    if (v->Sample(s, v0))
    {
      s = p + step;
      if (v->Sample(s, v1))
        dZ = (v1 - v0) * (1.0 / (2.0 * h));
      else
        dZ = (velocity - v0) * (1.0 / h);
    }
    else
    {
      s = p + step;
      v->Sample(s, v1);
      dZ = (v1 - velocity) * (1.0 / h);
    }

    vec3f curl(dY.z - dZ.y, dZ.x - dX.z, dX.y - dY.x);
    twist = (velocity * curl) / 4.0;

    float sint = sin(twist);
    float cost = cos(twist);

    float x = last_tangent.x * last_tangent.x;
    float y = last_tangent.y * last_tangent.y;
    float z = last_tangent.z * last_tangent.z;
    float xsq = x * x;
    float ysq = y * y;
    float zsq = z * z;

    float M[9] = {
      xsq * ( 1.0F - cost ) + cost,
      x * y * ( 1.0F - cost ) - z * sint,
      z * x * ( 1.0F - cost ) + y * sint,
      x * y * ( 1.0F - cost ) + z * sint,
      ysq * ( 1.0F - cost ) + cost,
      y * z * ( 1.0F - cost ) - x * sint,
      z * x * ( 1.0F - cost ) - y * sint,
      y * z * ( 1.0F - cost ) + x * sint,
      zsq * ( 1.0F - cost ) + cost
    };

    vec3f uN(u.x*M[0] + u.y*M[3] + u.z*M[6], u.x*M[1] + u.y*M[4] + u.z*M[7], u.x*M[2] + u.y*M[5] + u.z*M[8]);

    vec3f r;
    cross(normalized_velocity, uN, r);
    cross(r, normalized_velocity, u);
    normalize(u);

    next = v->PointOwner(p);
    if (next !=  me)
      break;

    uLast = u;
    pLast = p;
    tLast = t;
  }

  Lock();

  std::map<int, trajectory>::iterator ti = trajectories.find(id);
  if (ti == trajectories.end())
  {
    trajectory traj = shared_ptr<vector<segment>>(new vector<segment>);
    traj->push_back(seg);
    trajectories[id] = traj;
  }
  else
  {
    ti->second->push_back(seg);
  }

  Unlock();

  if (terminated || next == -1)
  {
    RKTraceCompleteMsg msg(getkey(), id);
    msg.Send(0);
  }
  else
    _Trace(next, id, n, pLast, uLast, tLast);
}

}
