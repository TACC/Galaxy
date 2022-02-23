#include <mpi.h>

#include "Application.h"
#include "PathLines.h"
#include "Work.h"

#include "RungeKutta.h"

namespace gxy
{

class TraceToPathLinesMsg : public Work
{
public:

  TraceToPathLinesMsg(RungeKuttaP rkp, PathLinesP plp, float t, float dt)
    : TraceToPathLinesMsg(2*sizeof(Key) + 2*sizeof(float))
  {
    unsigned char *g = (unsigned char *)get();
    *(Key *)g = rkp->getkey();
    g += sizeof(Key);
    *(Key *)g = plp->getkey();
    g += sizeof(Key);
    *(float *)g = t;
    g += sizeof(float);
    *(float *)g = dt;
    g += sizeof(float);
  }

  ~TraceToPathLinesMsg() {}

  WORK_CLASS(TraceToPathLinesMsg, true)

public:
  bool CollectiveAction(MPI_Comm c, bool is_root)
  {
    unsigned char *g = (unsigned char *)get();
    RungeKuttaP rkp = RungeKutta::GetByKey(*(Key *)g);
    g += sizeof(Key);
    PathLinesP plp = PathLines::GetByKey(*(Key *)g);
    g += sizeof(Key);
    float t = *(float *)g;
    g += sizeof(float);
    float dt = *(float *)g;
    g += sizeof(float);

    plp->CopyPartitioning(rkp);

    std::vector<int> keys;
    rkp->get_keys(keys);

    plp->clear();

    // How much space do I need?

    int np = 0, nc = 0, ii = 0;
    for (auto id = keys.begin(); id != keys.end(); id++, ii++)
    {
      trajectory traj = rkp->get_trajectory(*id);
      for (auto segp = traj->begin(); segp != traj->end(); segp++)
      {
        auto seg = *segp;

        auto k = 0;
        for (auto n = 0; n < seg->times.size(); n++)
          if (seg->times[n] > (t - dt) && seg->times[n] < t)
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
      trajectory traj = rkp->get_trajectory(*id);
      for (auto segp = traj->begin(); segp != traj->end(); segp++)
      {
        auto seg = *segp;
        auto k = 0, ik = -1;
        for (auto n = 0; n < seg->times.size(); n++)
          if (seg->times[n] > (t - dt) && seg->times[n] < t)
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

WORK_CLASS_TYPE(TraceToPathLinesMsg)

void
RegisterTraceToPathLines()
{
  TraceToPathLinesMsg::Register();
}

void
TraceToPathLines(RungeKuttaP rkp, PathLinesP plp, float t, float dt)
{
  TraceToPathLinesMsg msg(rkp, plp, t, dt);
  msg.Broadcast(true, true);
}

}
