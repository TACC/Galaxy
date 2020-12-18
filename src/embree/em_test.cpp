#include <Application.h>

#include "Embree.h"
#include "Triangles.h"
#include "EmbreeTriangles.h"
#include "Embree_ispc.h"

#include <memory>
#include <cstdlib>
#include <cmath>

void *
aligned_alloc(int bytes, int size, void* &ptr)
{
    int s = 1 << bytes;
    long base = (long)malloc(size+s);
    ptr = (void *)((base + s) & ~(s-1));
    return (void *)base;
}

using namespace gxy;
using namespace std;

int mpiRank = 0, mpiSize = 1;
#include "Debug.h"

class SetupGeometry : public Work
{
public:
    SetupGeometry(TrianglesP tp) : SetupGeometry(sizeof(Key))
    {
        unsigned char *p = (unsigned char *)get();

        *(Key *)p = tp->getkey();
        p = p + sizeof(Key);
    }

    ~SetupGeometry() {}
    WORK_CLASS(SetupGeometry, true)

public:
    bool CollectiveAction(MPI_Comm c, bool isRoot)
    {   
        unsigned char *p = (unsigned char *)get();

        TrianglesP tp = Triangles::GetByKey(*(Key *)p);

        tp->allocate_vertices(4);
        tp->allocate_connectivity(2*3);

        vec3f *vertices = tp->GetVertices();

        vertices[0].x = mpiRank + 0.f; vertices[0].y = 0.f; vertices[0].z  = 0.f;
        vertices[1].x = mpiRank + 1.f; vertices[1].y = 0.f; vertices[1].z  = 0.f;
        vertices[2].x = mpiRank + 1.f; vertices[2].y = 1.f; vertices[2].z  = 0.f;
        vertices[3].x = mpiRank + 0.f; vertices[3].y = 1.f; vertices[3].z = 0.f;

        float hpi = 3.1415926 / 2.0;
        float qpi = 3.1415926 / 4.0;

        float d0 = -qpi + (float(mpiRank) / mpiSize)*hpi;
        float c0 = cos(d0);
        float s0 = sin(d0);

        float d1 = -qpi + (float(mpiRank+1) / mpiSize)*hpi;
        float c1 = cos(d1);
        float s1 = sin(d1);
        
        vec3f *normals = tp->GetNormals();

        normals[0].x = s0; normals[0].y = 0.f; normals[0].z = -c0;
        normals[1].x = s1; normals[1].y = 0.f; normals[1].z = -c1;
        normals[2].x = s1; normals[2].y = 0.f; normals[2].z = -c1;
        normals[3].x = s0; normals[3].y = 0.f; normals[3].z = -c0;

        int *indices = tp->GetConnectivity();

        indices[0] = 0; indices[1] = 1; indices[2] = 3;
        indices[3] = 1; indices[4] = 2; indices[5] = 3;

        MPI_Barrier(c);
        
        return false;
    }
}; 

class GeomToEmbree : public Work
{
public:
    GeomToEmbree(TrianglesP tp) : GeomToEmbree(sizeof(Key))
    {
        unsigned char *p = (unsigned char *)get();

        *(Key *)p = tp->getkey();
        p = p + sizeof(Key);
    }

    ~GeomToEmbree() {}
    WORK_CLASS(GeomToEmbree, true)

public:
    bool CollectiveAction(MPI_Comm c, bool isRoot)
    {   
        unsigned char *p = (unsigned char *)get();
        TrianglesP tp = Triangles::GetByKey(*(Key *)p);

        EmbreeTrianglesP etp = EmbreeTriangles::NewP(tp);
        GetEmbree()->AddGeometry(etp);

        MPI_Barrier(c);
        
        return false;
    }
};

class IntersectMsg : public Work
{
public:
    IntersectMsg(float x, float y, float z) : IntersectMsg(3*sizeof(float))
    {
        float *p = (float *)get();
        p[0] = x;
        p[1] = y;
        p[2] = z;
    }

    ~IntersectMsg() {}
    WORK_CLASS(IntersectMsg, true)

public:
    bool CollectiveAction(MPI_Comm c, bool isRoot)
    {   
        float *p = (float *)get();

        void *rbase; RTCRayHit8 *raypkt;
        rbase = aligned_alloc(8, sizeof(RTCRayHit8), (void *&)raypkt);

        void *vbase; int *valid;
        vbase = aligned_alloc(8, 8*sizeof(int), (void *&)valid);

        for (int i = 0; i < 8; i++)
            valid[i] = -1;

        for (int i = 0; i < 8; i++)
        {
            raypkt->ray.org_x[i] = i + p[0];
            raypkt->ray.org_y[i] = p[1];
            raypkt->ray.org_z[i] = p[2];
            raypkt->ray.dir_x[i] = 0;
            raypkt->ray.dir_y[i] = 0;
            raypkt->ray.dir_z[i] = 1;
            raypkt->ray.tnear[i] = 0;
            raypkt->ray.tfar[i] = std::numeric_limits<float>::infinity();
            raypkt->ray.mask[i] = -1;
            raypkt->ray.flags[i] = 0;
            raypkt->hit.geomID[i] = RTC_INVALID_GEOMETRY_ID;
            raypkt->hit.instID[0][i] = RTC_INVALID_GEOMETRY_ID;
        }

        for (int i = 0; i < 8; i++)
            valid[i] = -1;

        ispc::em_Intersect((RTCScene)GetEmbree()->Scene(), valid, 1, (void *)raypkt);

        if (mpiRank == 0)
            std::cout << "Indirect call through ISPC\n";

        for (int i = 0; i < 8; i++)
        {
            if (i == mpiRank && raypkt->hit.geomID[i] != RTC_INVALID_GEOMETRY_ID)
            { 
               std::cout 
                   << "proc " << mpiRank 
                   << "g " << raypkt->hit.geomID[i] 
                   << "p " << raypkt->hit.primID[i] 
                   << "t " << raypkt->ray.tfar[i] 
                   << "u " << raypkt->hit.u[i] 
                   << "v " << raypkt->hit.v[i];
            }
        }
        
        free(vbase);
        free(rbase);

        MPI_Barrier(c);
        
        return false;
    }
};

WORK_CLASS_TYPE(SetupGeometry)
WORK_CLASS_TYPE(GeomToEmbree)
WORK_CLASS_TYPE(IntersectMsg)

void
syntax(char *a)
{
  if (mpiRank == 0)
  {
    std::cerr << "syntax: " << a << " [options] " << endl;
    std::cerr << "options:" << endl;
    std::cerr << "  -D[which]  run debugger in selected processes.  If which is given, it is a number or a hyphenated range, defaults to all" << endl;
  }
  exit(1);
}

int
main(int argc, char *argv[])
{
    char *dbgarg;
    bool dbg = false;
    bool atch = false;

    Application theApplication(&argc, &argv);
    theApplication.Start();

    for (int i = 1; i < argc; i++)
        if (!strncmp(argv[i],"-D", 2)) dbg = true, atch = false, dbgarg = argv[i] + 2;
        else syntax(argv[0]);

    mpiRank = theApplication.GetRank();
    mpiSize = theApplication.GetSize();

    Debug *d = dbg ? new Debug(argv[0], atch, dbgarg) : NULL;

    theApplication.Run();

    Embree::Register();
    KeyedDataObject::Register();
    SetupGeometry::Register();
    GeomToEmbree::Register();
    IntersectMsg::Register();

    if (mpiRank == 0)
    {
        EmbreeP ep = Embree::NewP();
        TrianglesP tp = Triangles::NewP();

        SetupGeometry sg(tp);
        sg.Broadcast(true, true);

        GeomToEmbree g2e(tp);
        g2e.Broadcast(true, true);

        ep->Commit();

        IntersectMsg i = IntersectMsg(0.1, 0.1, -1);
        i.Broadcast(true, true);

        theApplication.QuitApplication();
    }

    theApplication.Wait();
}
