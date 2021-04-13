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
bool show_Z = false;

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

        normals[0].x = mpiRank + 0.f; normals[0].y = 1.f; normals[0].z = 0.f;
        normals[1].x = mpiRank + 1.f; normals[1].y = 0.f; normals[1].z = 0.f;
        normals[2].x = mpiRank + 1.f; normals[2].y = 0.f; normals[2].z = 0.f;
        normals[3].x = mpiRank + 0.f; normals[3].y = 1.f; normals[3].z = 0.f;

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
        data[0] = 0.20 - ((float)mpiRank/mpiSize) * 0.02;
        data[1] = 0.20 - ((float)mpiRank/mpiSize) * 0.02;
        data[2] = 0.20 - ((float)mpiRank/mpiSize) * 0.02;
        data[3] = 0.20 - ((float)mpiRank/mpiSize) * 0.02;

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

        return false;
    }
}; 

void Intersect(ModelPtr model, RayList *rays)
{
    ::ispc::Test_Intersect(IntelModel::Cast(model)->GetDeviceEquivalent(), rays->GetRayCount(), rays->GetIspc());
}

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

        for (auto g : geometries)
          Device::GetTheDevice()->CreateTheDatasetDeviceEquivalent(g);

        ModelPtr model = Device::GetTheDevice()->NewModel();
        for (auto g : geometries)
        {
          GeometryVisDPtr gv = GeometryVis::New();
          model->AddGeometry(g, gv);
        }

        model->Build();
        
        RayList *rays = new RayList(50*mpiSize);

        for (int i = 0; i < 50*mpiSize; i++)
        {
            rays->get_ox_base()[i]   = (1/50.0)*i + p[0];
            rays->get_oy_base()[i]   = p[1];
            rays->get_oz_base()[i]   = p[2];
            rays->get_dx_base()[i]   = 0;
            rays->get_dy_base()[i]   = 0;
            rays->get_dz_base()[i]   = 1;
            rays->get_t_base()[i]    = 0;
            rays->get_term_base()[i] = -1;
            rays->get_tMax_base()[i] = std::numeric_limits<float>::infinity();
        }

        // model->Intersect(rays);
        Intersect(model, rays);

        for (int j = 0; j < mpiSize; j++)
        {
            MPI_Barrier(c);

            if (j == mpiRank)
            {
                for (int i = 0; i < 50*mpiSize; i++)
                {
                    char c;
                    switch (rays->get_term_base()[i])
                    {
                        case 0: c = '0'; break;
                        case 1: c = '1'; break;
                        case 2: c = '2'; break;
                        case 3: c = '3'; break;
                        default: c = ' ';
                    }
                    std::cerr << c;
                }
                std::cerr << "\n";
                if (show_Z)
                {
                for (int i = 0; i < 50*mpiSize; i++)
                    if (rays->get_term_base()[i] != -1)
                    {
                        float t = rays->get_tMax_base()[i];
                        std::cerr << i << ": " 
                            << "(" << (rays->get_ox_base()[i] + t*(rays->get_dx_base()[i]))
                            << " " << (rays->get_oy_base()[i] + t*(rays->get_dy_base()[i]))
                            << " " << (rays->get_oz_base()[i] + t*(rays->get_dz_base()[i]))
                            << ") (" << rays->get_nx_base()[i] 
                            << " " << rays->get_ny_base()[i] 
                            << " " << rays->get_nz_base()[i] 
                            << ") " << rays->get_tMax_base()[i] << "\n";
                    }
                }
            }

            // sleep(1);
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
    std::cerr << "  -y y       vertical position of samples (0.5)" << endl;
    std::cerr << "  -z         show hit points, normals, T" << endl;
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
        else if (!strcmp(argv[i], "-y")) y = atof(argv[++i]);
        else if (!strcmp(argv[i], "-z")) show_Z = true;
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

            IntersectMsg i = IntersectMsg(0.0, y, -1);
            i.Broadcast(true, true);
        }

        theApplication.QuitApplication();
    }

    theApplication.Wait();

    IntelDevice::FreeDevice();
}
