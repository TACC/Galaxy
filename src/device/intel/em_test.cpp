#include <Application.h>
#include <DataObjects.h>
#include <Rays.h>

#include "test_ispc.h"

#include "Triangles.h"
#include "Particles.h"
#include "PathLines.h"

#include "IntelDevice.h"
#include "IntelModel.h"

#include "EmbreeTriangles.h"
#include "EmbreeSpheres.h"
#include "EmbreePathLines.h"

#include <memory>
#include <cstdlib>
#include <cmath>

int test_element_types = 0;
int RES = 50;

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

std::vector<GeometryDPtr> geometries;
std::vector<VolumeDPtr>   volumes;

class SetupTriangles : public Work
{
public:
    SetupTriangles(TrianglesDPtr tp) : SetupTriangles(sizeof(Key))
    {
        Key *p = (Key *)get();
        p[0] = tp->getkey();
    }

    ~SetupTriangles() {}
    WORK_CLASS(SetupTriangles, true)

public:
    bool CollectiveAction(MPI_Comm c, bool isRoot)
    {   
        Key *p = (Key *)get();
        
        TrianglesDPtr tp = Triangles::GetByKey(p[0]);

        tp->allocate_vertices(4);
        tp->allocate_connectivity(2*3);

        vec3f *vertices = tp->GetVertices();

        vertices[0].x = mpiRank + 0.1f; vertices[0].y = 0.1f; vertices[0].z = 0.f;
        vertices[1].x = mpiRank + 0.9f; vertices[1].y = 0.1f; vertices[1].z = 0.f;
        vertices[2].x = mpiRank + 0.9f; vertices[2].y = 0.9f; vertices[2].z = 0.f;
        vertices[3].x = mpiRank + 0.1f; vertices[3].y = 0.9f; vertices[3].z = 0.f;

        vec3f *normals = tp->GetNormals();

        normals[0].x = mpiRank + 0.f; normals[0].y = 0.f; normals[0].z = -1.f;
        normals[1].x = mpiRank + 0.f; normals[1].y = 0.f; normals[1].z = -1.f;
        normals[2].x = mpiRank + 0.f; normals[2].y = 0.f; normals[2].z = -1.f;
        normals[3].x = mpiRank + 0.f; normals[3].y = 0.f; normals[3].z = -1.f;

        int *indices = tp->GetConnectivity();

        indices[0] = 0; indices[1] = 1; indices[2] = 3;
        indices[3] = 1; indices[4] = 2; indices[5] = 3;

        geometries.push_back(tp);

        return false;
    }
}; 

class SetupSpheres : public Work
{
public:
    SetupSpheres(ParticlesDPtr pp) : SetupSpheres(sizeof(Key))
    {
        Key *p = (Key *)get();

        p[0] = pp->getkey();
    }

    ~SetupSpheres() {}
    WORK_CLASS(SetupSpheres, true)

public:
    bool CollectiveAction(MPI_Comm c, bool isRoot)
    {   
        Key *p = (Key *)get();
        
        ParticlesDPtr pp = Particles::GetByKey(p[0]);

        pp->allocate_vertices(4);
        pp->allocate_connectivity(4);

        vec3f *vertices = pp->GetVertices();
        float *data = pp->GetData();

        vertices[0].x = mpiRank + 0.25; vertices[0].y = 0.25; vertices[0].z = 0.0;
        vertices[1].x = mpiRank + 0.25; vertices[1].y = 0.75; vertices[1].z = 0.0;
        vertices[2].x = mpiRank + 0.75; vertices[2].y = 0.75; vertices[2].z = 0.0;
        vertices[3].x = mpiRank + 0.75; vertices[3].y = 0.25; vertices[3].z = 0.0;
        data[0] = 0.10;
        data[1] = 0.15;
        data[2] = 0.20;
        data[3] = 0.25;

        geometries.push_back(pp);
        
        return false;
    }
}; 

class SetupPathLines : public Work
{
public:
    SetupPathLines(PathLinesDPtr plp) : SetupPathLines(sizeof(Key))
    {
        Key *p = (Key *)get();
        p[0] = plp->getkey();
    }

    ~SetupPathLines() {}
    WORK_CLASS(SetupPathLines, true)

public:
    bool CollectiveAction(MPI_Comm c, bool isRoot)
    {   
        Key *p = (Key *)get();
        
        PathLinesDPtr plp = PathLines::GetByKey(p[0]);

        plp->allocate_vertices(4);
        plp->allocate_connectivity(3);

        vec3f *vertices = plp->GetVertices();

#if 0
        vertices[0].x = mpiRank + 0.0f; vertices[0].y = 1.f; vertices[0].z = 0.f;
        vertices[1].x = mpiRank + 0.3f; vertices[1].y = 0.f; vertices[1].z = 0.f;
        vertices[2].x = mpiRank + 0.7f; vertices[2].y = 0.f; vertices[2].z = 0.f;
        vertices[3].x = mpiRank + 1.0f; vertices[3].y = 1.f; vertices[3].z = 0.f;

        int *indices = plp->GetConnectivity();

        indices[0] = 0; 
        indices[1] = 1; 
        indices[2] = 2;

        float *data = plp->GetData();

        data[0] = 0.10;
        data[1] = 0.15;
        data[2] = 0.20;
        data[3] = 0.25;

        geometries.push_back(plp);
#else
        vertices[0].y = mpiRank + 0.0f; vertices[0].x = 0.5f; vertices[0].z = 0.f;
        vertices[1].y = mpiRank + 0.3f; vertices[1].x = 0.5f; vertices[1].z = 0.f;
        vertices[2].y = mpiRank + 0.7f; vertices[2].x = 0.5f; vertices[2].z = 0.f;
        vertices[3].y = mpiRank + 1.0f; vertices[3].x = 0.5f; vertices[3].z = 0.f;

        int *indices = plp->GetConnectivity();

        indices[0] = 0; 
        indices[1] = 1; 
        indices[2] = 2;

        float *data = plp->GetData();

        data[0] = 0.10;
        data[1] = 0.15;
        data[2] = 0.20;
        data[3] = 0.25;

        geometries.push_back(plp);
#endif

        return false;
    }
}; 

void Intersect(DeviceModelPtr model, RayList *rays)
{
    ::ispc::Test_Intersect(IntelModel::Cast(model)->GetDeviceEquivalent(), rays->GetRayCount(), rays->GetIspc());
}

class IntersectMsg : public Work
{
public:

    ~IntersectMsg() {}
    WORK_CLASS(IntersectMsg, true)

public:
    bool CollectiveAction(MPI_Comm c, bool isRoot)
    {   
        for (auto g : geometries)
          Device::GetTheDevice()->CreateTheDatasetDeviceEquivalent(g);

        DeviceModelPtr model = Device::GetTheDevice()->NewDeviceModel();
        for (auto g : geometries)
        {
          GeometryVisDPtr gv = GeometryVis::New();
          model->AddGeometry(g, gv);
        }

        model->Build();
        
        RayList *rays = new RayList(1024);
        bool first = true;

        int k = 0, v = 0;
        for (int i = 0; i < RES; i++)
          for (int j = 0; j < RES; j++, v++)
          {
            rays->get_ox_base()[k]   = mpiRank + (1/(RES-1.0))*i;
            rays->get_oy_base()[k]   = (1/(RES-1.0))*j;
            rays->get_oz_base()[k]   = -1;
            rays->get_dx_base()[k]   = 0;
            rays->get_dy_base()[k]   = 0;
            rays->get_dz_base()[k]   = 1;
            rays->get_t_base()[k]    = 0;
            rays->get_term_base()[k] = -1;
            rays->get_tMax_base()[k] = std::numeric_limits<float>::infinity();
            k++;

            if (k == 1024)
            {
              Intersect(model, rays);

              for (int j = 0; j < mpiSize; j++)
              {
                if (first)
                {
                  first = false;
                  std::cout << "x,y,z,nx,ny,nz,i\n";
                }

                for (int i = 0; i < rays->GetRayCount(); i++)
                  if (rays->get_term(i))
                  {
                      float t = rays->get_tMax(i);
                      float x = rays->get_ox(i) + t*rays->get_dx(i);
                      float y = rays->get_oy(i) + t*rays->get_dy(i);
                      float z = rays->get_oz(i) + t*rays->get_dz(i);

                      std::cout << x << "," << y << "," << z << "," <<
                                   rays->get_nx(i) << "," << rays->get_ny(i) << "," << rays->get_nz(i) << "," << i << "\n";
                  }
              }

              k = 0;
            }
          }

        return false;
    }
};

WORK_CLASS_TYPE(SetupTriangles)
WORK_CLASS_TYPE(SetupSpheres)
WORK_CLASS_TYPE(SetupPathLines)
WORK_CLASS_TYPE(IntersectMsg)

void
syntax(char *a)
{
  if (mpiRank == 0)
  {
    std::cerr << "syntax: " << a << " [options] " << endl;
    std::cerr << "options:" << endl;
    std::cerr << "  -D[which]  run debugger in selected processes.  If which is given, it is a number or a hyphenated range, defaults to all" << endl;
    std::cerr << "  -t types   test element types,   bitfield 1 for triangles, 2 for spheres, 4 for pathlines.  Multiple -t's are OR'd" << endl;
    std::cerr << "  -R res     number of samples along edge (50)\n";
  }
  exit(1);
}

int
main(int argc, char *argv[])
{
    char *dbgarg;
    bool dbg = false;
    bool atch = false;
    float y = 0.5;

    IntelDevice::InitDevice();

    Application theApplication(&argc, &argv);

    RegisterDataObjects();

    theApplication.Start();

    for (int i = 1; i < argc; i++)
        if (!strncmp(argv[i], "-D", 2)) dbg = true, atch = false, dbgarg = argv[i] + 2;
        else if (!strcmp(argv[i], "-t")) test_element_types |= atoi(argv[++i]);
        else if (!strcmp(argv[i], "-R")) RES = atoi(argv[++i]);
        else syntax(argv[0]);

    mpiRank = theApplication.GetRank();
    mpiSize = theApplication.GetSize();

    Debug *d = dbg ? new Debug(argv[0], atch, dbgarg) : NULL;

    theApplication.Run();

    SetupTriangles::Register();
    SetupSpheres::Register();
    SetupPathLines::Register();
    IntersectMsg::Register();

    if (mpiRank == 0)
    {
        {
            TrianglesDPtr tp;
            if (test_element_types & 0x1)
            {
                tp = Triangles::NewDistributed();
                SetupTriangles s(tp);
                s.Broadcast(true, true);
                tp->Commit();
            }
                   
            ParticlesDPtr pp;
            if (test_element_types & 0x2)
            {
                pp = Particles::NewDistributed();
                SetupSpheres s(pp);
                s.Broadcast(true, true);
                pp->Commit();
            }
               
            PathLinesDPtr plp;
            if (test_element_types & 0x4)
            {
                plp = PathLines::NewDistributed();
                SetupPathLines s(plp);
                s.Broadcast(true, true);
                plp->Commit();
            }

            IntersectMsg i = IntersectMsg();
            i.Broadcast(true, true);
        }

        theApplication.QuitApplication();
    }

    theApplication.Wait();

    IntelDevice::FreeDevice();
}
