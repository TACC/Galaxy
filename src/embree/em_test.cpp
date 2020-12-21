#include <Application.h>
#include <Rays.h>

#include "Embree.h"
#include "EmbreeModel.h"
#include "Triangles.h"
#include "EmbreeTriangles.h"

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
    GeomToEmbree(EmbreeModelP emp, GeometryP tp) : GeomToEmbree(2*sizeof(Key))
    {
        unsigned char *p = (unsigned char *)get();

        *(Key *)p = emp->getkey();
        p = p + sizeof(Key);

        *(Key *)p = tp->getkey();
        p = p + sizeof(Key);
    }

    ~GeomToEmbree() {}
    WORK_CLASS(GeomToEmbree, true)

public:
    bool CollectiveAction(MPI_Comm c, bool isRoot)
    {   
        Key *p = (Key *)get();
        EmbreeModelP emp = EmbreeModel::GetByKey(p[0]);
        GeometryP gp = Geometry::GetByKey(p[1]);

        if (Triangles::Cast(gp))
        {
            EmbreeTrianglesP etp = EmbreeTriangles::NewL();
            etp->SetGeometry(gp);
            emp->AddGeometry(etp);
        }

        MPI_Barrier(c);
        
        return false;
    }
};

class IntersectMsg : public Work
{
public:
    IntersectMsg(EmbreeP ep, EmbreeModelP emp, float x, float y, float z) : IntersectMsg(2*sizeof(Key) + 3*sizeof(float))
    {
        Key *k = (Key *)get();
        k[0] = ep->getkey();
        k[1] = emp->getkey();
        float *p = (float *)(k + 2);
        
        p[0] = x;
        p[1] = y;
        p[2] = z;
    }

    ~IntersectMsg() {}
    WORK_CLASS(IntersectMsg, true)

public:
    bool CollectiveAction(MPI_Comm c, bool isRoot)
    {   
        std::cerr << "XXXXXXXX " << mpiRank << "\n";

        Key *k = (Key *)get();
        EmbreeP ep = Embree::GetByKey(k[0]);
        EmbreeModelP emp = EmbreeModel::GetByKey(k[1]);

        float *p = (float *)(k + 2);

        RayList *rays = new RayList(100);

        for (int i = 0; i < 100; i++)
        {
            rays->get_ox_base()[i]   = (0.1)*i + p[0];
            rays->get_oy_base()[i]   = p[1];
            rays->get_oz_base()[i]   = p[2];
            rays->get_dx_base()[i]   = 0;
            rays->get_dy_base()[i]   = 0;
            rays->get_dz_base()[i]   = 1;
            rays->get_t_base()[i]    = 0;
            rays->get_term_base()[i] = -1;
            rays->get_tMax_base()[i] = std::numeric_limits<float>::infinity();
        }

        ep->Intersect(emp, 100, rays);

        if (mpiRank == 0)
            std::cerr << "Indirect call through ISPC\n";

        for (int j = 0; j < mpiSize; j++)
        {
            MPI_Barrier(c);

            if (j == mpiRank)
            {
                std::cerr << "proc " << mpiRank << "\n";
                for (int i = 0; i < 100; i++)
                    std::cerr << (rays->get_term_base()[i] ? " " : "X");
                std::cerr << "\n";
                for (int i = 0; i < 100; i++)
                    if (rays->get_term_base()[i] != -1)
                        std::cerr << i << ": " << rays->get_t_base()[i] << "\n";
            }

            sleep(1);
        }
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
    std::cerr << "r " << mpiRank << "\n";


    Embree::Register();
    EmbreeModel::Register();
    EmbreeGeometry::Register();
    EmbreeTriangles::Register();

    KeyedDataObject::Register();

    SetupGeometry::Register();
    GeomToEmbree::Register();
    IntersectMsg::Register();

    if (mpiRank == 0)
    {
        EmbreeP ep = Embree::NewP();
        ep->Commit();

        theApplication.SyncApplication();
        std::cerr << "11111111\n";

        EmbreeModelP emp = EmbreeModel::NewP();
        emp->SetEmbree(ep);
        emp->Commit();

        theApplication.SyncApplication();
        std::cerr << "222222222\n";

        TrianglesP tp = Triangles::NewP();

        SetupGeometry sg(tp);
        sg.Broadcast(true, true);

        std::cerr << "33333333\n";

        GeomToEmbree g2e(emp, tp);
        g2e.Broadcast(true, true);

        std::cerr << "4444444444\n";

        emp->Commit();

        IntersectMsg i = IntersectMsg(ep, emp, 0.1, 0.1, -1);
        i.Broadcast(true, true);

        theApplication.QuitApplication();
    }

    theApplication.Wait();
}
