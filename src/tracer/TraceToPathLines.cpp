#include <mpi.h>

#include <gxy/Application.h>
#include <gxy/PathLines.h>
#include <gxy/Work.h>

#include "RungeKutta.h"

namespace gxy
{

class TraceToPathLinesMsg : public Work
{
public:

  TraceToPathLinesMsg(VolumeP vp, RungeKuttaP rkp, PathLinesP plp) 
    : TraceToPathLinesMsg(3*sizeof(Key))
  {
    unsigned char *g = (unsigned char *)get();
    *(Key *)g = vp->getkey();
    g += sizeof(Key);
    *(Key *)g = rkp->getkey();
    g += sizeof(Key);
    *(Key *)g = plp->getkey();
    g += sizeof(Key);
  }

  ~TraceToPathLinesMsg() {}

  WORK_CLASS(TraceToPathLinesMsg, true)

public:
  bool CollectiveAction(MPI_Comm c, bool is_root)
  {
    unsigned char *g = (unsigned char *)get();
    VolumeP vp = Volume::GetByKey(*(Key *)g);
    g += sizeof(Key);
    RungeKuttaP rkp = RungeKutta::GetByKey(*(Key *)g);
    g += sizeof(Key);
    PathLinesP plp = PathLines::GetByKey(*(Key *)g);
    g += sizeof(Key);

    plp->CopyPartitioning(vp);

    std::vector<int> keys;
    rkp->get_keys(keys);

    plp->clear();

    // How much space do I need?

    int np = 0, nc = 0;
    for (auto id = keys.begin(); id != keys.end(); id++)
    {
      trajectory t = rkp->get_trajectory(*id);
      for (auto seg = t->begin(); seg != t->end(); seg++)
      {
        np += (*seg)->points.size();
        nc += (*seg)->points.size() - 1;
      }
    }

    plp->allocate(np, nc);
    
    vec3f *pbuf = plp->GetVertices();
    float *dbuf = plp->GetData();
    int   *cbuf = plp->GetConnectivity();

    np = 0; nc = 0;
    for (auto id = keys.begin(); id != keys.end(); id++)
    {
      trajectory t = rkp->get_trajectory(*id);
      for (auto seg = t->begin(); seg != t->end(); seg++)
      {
        memcpy((void *)(pbuf + np), (*seg)->points.data(), (*seg)->points.size()*sizeof(vec3f));
        memcpy((void *)(dbuf + np), (*seg)->times.data(),   (*seg)->times.size()*sizeof(float));
        for (int i = 0; i < (*seg)->points.size(); i++, np++)
          if (i < ((*seg)->points.size() - 1))
            cbuf[nc++] = np;
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
TraceToPathLines(VolumeP vp, RungeKuttaP rkp, PathLinesP plp)
{
  TraceToPathLinesMsg msg(vp, rkp, plp);
  msg.Broadcast(true, true);
}

}
