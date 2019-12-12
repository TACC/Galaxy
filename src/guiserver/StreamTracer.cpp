#include "Threading.h"
#include "Application.h"

#include "StreamTracer.hpp"

#include <pthread.h>

using namespace std;

namespace gxy
{

WORK_CLASS_TYPE(StreamTracer::StreamTracerMsg)
WORK_CLASS_TYPE(StreamTracer::StreamTracerCompleteMsg)
WORK_CLASS_TYPE(StreamTracer::StreamTracerCountMsg)
WORK_CLASS_TYPE(StreamTracer::StreamTracerFromParticleSetMsg)
WORK_CLASS_TYPE(StreamTracer::StreamTracerTraceToPathLinesMsg)

KEYED_OBJECT_CLASS_TYPE(StreamTracer)

void
StreamTracer::Register()
{
  RegisterClass();
  StreamTracerMsg::Register();
  StreamTracerCompleteMsg::Register();
  StreamTracerCountMsg::Register();
  StreamTracerFromParticleSetMsg::Register();
  StreamTracerTraceToPathLinesMsg::Register();
}

void
StreamTracer::initialize()
{
  super::initialize();
  pthread_cond_init(&signal, NULL);
  pthread_mutex_init(&lock, NULL);
}

int 
StreamTracer::serialSize() { return super::serialSize() + sizeof(Key) + sizeof(int) + 5*sizeof(float); }

bool
StreamTracerFilter::SetVectorField(VolumeP v)
{
  if (!v || 
      v->get_number_of_components() != 3 || 
      v->get_type() != Volume::FLOAT)
  {
    std::cerr << "StreamTracer::SetVectorField:  bad vector field\n";
    return false;
  }
  vectorField = v;
  return true;
}

unsigned char *
StreamTracer::serialize(unsigned char *ptr)
{
  ptr = super::serialize(ptr);
  *(Key *)ptr = vectorField->getkey(); ptr += sizeof(Key);
  *(int *)ptr = max_steps; ptr += sizeof(int);
  *(float *)ptr = stepsize; ptr += sizeof(float);
  *(float *)ptr = min_velocity; ptr += sizeof(float);
  *(float *)ptr = max_integration_time; ptr += sizeof(float);
  *(float *)ptr = tStart; ptr += sizeof(float);
  *(float *)ptr = deltaT; ptr += sizeof(float);

  return ptr;
}

unsigned char *
StreamTracer::deserialize(unsigned char *ptr)
{
  ptr = super::deserialize(ptr);
  vectorField = Volume::GetByKey(*(Key *)ptr); ptr += sizeof(Key);
  max_steps = *(int *)ptr; ptr += sizeof(int);
  stepsize = *(float *)ptr; ptr += sizeof(float);
  min_velocity = *(float *)ptr; ptr += sizeof(float);
  max_integration_time = *(float *)ptr; ptr += sizeof(float);
  tStart = *(float *)ptr; ptr += sizeof(float);
  deltaT = *(float *)ptr; ptr += sizeof(float);

  return ptr;
}

void
StreamTracer::Trace(vec3f& p, int id)
{
  Lock();

  in_flight = 1;
  max_integration_time = 0;
  
  vec3f u(0.0, 1.0, 0.0);
  _Trace(GetVectorField()->PointOwner(p), id, 0, p, u, 0.0);

  while (in_flight) 
    Wait();

  Unlock();
}


void
StreamTracer::Trace(int n, vec3f* p)
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
StreamTracer::Trace(ParticlesP p)
{
  Lock();

  // Offset by a huge number so that the completion of 
  // traces before the actual number is known don't
  // make it think its done.   The message from the 
  // last process will remove this offset. 

  in_flight = STREAMTRACER_INFLIGHT_OFFSET;

  StreamTracerFromParticleSetMsg msg(getkey(), p, 0);
  msg.Send(0);

  while (in_flight) 
    Wait();

  Unlock();
}

void
StreamTracer::_Trace(int where, int id, int n, vec3f& p, vec3f& u, float t)
{
  if (where != -1)
  {
    StreamTracerMsg msg(getkey(), id, n, t, u, p);
    msg.Send(where);
  }
}

bool
StreamTracer::local_commit(MPI_Comm c)
{
  super::local_commit(c);
  CopyPartitioning(vectorField);
  return false;
}

void
StreamTracer::TraceToPathLines(PathLinesP plp)
{ 
  StreamTracerTraceToPathLinesMsg msg(this, plp);
  msg.Broadcast(true, true);
}

void
StreamTracer::local_trace(int id, int n, vec3f& p, vec3f& u, float t)
{
  segment seg(new _segment);
  vec3f pLast, uLast;
  float tLast;
  int me = GetTheApplication()->GetRank();

  VolumeP v = GetVectorField();

  // h will be the step size

  vec3f d;
  v->get_deltas(d.x, d.y, d.z);

  float h = stepsize * (d.x > d.y ? d.y > d.z ? d.z : d.y : d.x > d.z ? d.z : d.x);

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

    float vlen = len(velocity);
    if (min_velocity > 0 && vlen < min_velocity)
    {
      terminated = true;
      zero(velocity);
      zero(normalized_velocity);
    }
    else
    {
      if (max_integration_time >= 0  && max_integration_time < tLast)
        terminated = true;

      normalized_velocity = velocity * (1.0 / vlen);
    }

    float scaled_h = h / vlen;

    // If this is the first point, get an initial up as anything perpendicular to the 
    // initial velocity vector.   Otherwise, rotate the last one.

    float twist = 0;

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

    last_tangent = velocity;

    // Now take a Runge Kutta step.  The length of the step we will
    // actually take is h, regardless of the magnitude of the vectors.
    // So we normalize the sampled vectors and multiply by h to get the
    // RK vectors.

    vec3f k1, k2, k3, k4, rkvec;

    k1 = normalized_velocity * scaled_h;

    vec3f p2 = p + (k1 * 0.5), v2;
    v->Sample(p2, v2);
    normalize(v2);
    k2 = v2 * scaled_h;

    vec3f  p3 = p + (k2 * 0.5), v3;
    v->Sample(p3, v3);
    normalize(v3);
    k3 = v3 * scaled_h;

    vec3f  p4 = p + k3, v4;
    v->Sample(p4, v4);
    normalize(v4);
    k4 = v4 * scaled_h;

    rkvec = (k1 + (k2 * 2) + (k3 * 2) + k4) * (1.0/6.0);

    p = p + rkvec;
    t = t + scaled_h;

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
    std::cerr << "new traj " << id << "\n";
  }
  else
  {
    ti->second->push_back(seg);
    std::cerr << "added segment to traj " << id << "\n";
  }

  Unlock();

  if (terminated || next == -1)
  {
    StreamTracerCompleteMsg msg(getkey(), tLast);
    msg.Send(0);
  }
  else
    _Trace(next, id, n, pLast, uLast, tLast);
}

}
