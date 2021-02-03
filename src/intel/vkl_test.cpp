#include <Application.h>
#include <DataObjects.h>
#include <Rays.h>

#include "Volume.h"

#include "IntelDevice.h"
#include "IntelModel.h"

#include "VklVolume.h"

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

class SetupVolume : public Work
{
public:
    SetupVolume(VolumeDPtr tp) : SetupVolume(sizeof(Key))
    {
        Key *p = (Key *)get();
        p[0] = tp->getkey();
    }

    ~SetupVolume() {}
    WORK_CLASS(SetupVolume, true)

public:
    bool CollectiveAction(MPI_Comm c, bool isRoot)
    {   
        Key *p = (Key *)get();
        
        VolumeDPtr vp = gxy::Volume::GetByKey(p[0]);

        vp->set_global_partitions(1, 1, 1);
        vp->set_ijk(0, 0, 0);

        vp->set_type(gxy::Volume::FLOAT);
        vp->set_number_of_components(1);

        vp->set_global_origin(0.0, 0.0, 0.0);
        vp->set_ghosted_local_offset(0, 0, 0);
        vp->set_local_offset(1, 1, 1);
        vp->set_ghosted_local_counts(32, 32, 32);
        vp->set_deltas(1.0 / 31, 1.0 / 31, 1.0 / 31);
        vp->set_local_counts(30, 30, 30);

        vp->set_local_minmax(1.0/31.0, 1.0 - (1.0/31.0));

        vp->set_samples(malloc(32 * 32 * 32 * sizeof(float)));

        float *ptr = (float *)vp->get_samples().get();
        for (int i = 0; i < 32; i++)
          for (int j = 0; j < 32; j++)
            for (int k = 0; k < 32; k++)
              *ptr++ = (i/31.0);
        
        volumes.push_back(vp);

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

        ModelPtr model = Device::GetTheDevice()->NewModel();

        for (auto g : geometries)
        {
          Device::GetTheDevice()->CreateTheDatasetDeviceEquivalent(g);
          GeometryVisDPtr gv = GeometryVis::New();
          model->AddGeometry(g, gv);
        }

        for (auto v : volumes)
        {
          Device::GetTheDevice()->CreateTheDatasetDeviceEquivalent(v);
          VolumeVisDPtr vv = VolumeVis::New();
          model->AddVolume(v, vv);
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

        model->Intersect(rays);
#if 0
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
        }
#endif
        return false;
    }
};

class SampleMsg : public Work
{
public:
    SampleMsg(float x, float y, float z) : SampleMsg(3*sizeof(float))
    {
        float *p = (float *)get();
        
        p[0] = x;
        p[1] = y;
        p[2] = z;
    }

    ~SampleMsg() {}
    WORK_CLASS(SampleMsg, true)

public:
    bool CollectiveAction(MPI_Comm c, bool isRoot)
    {   
        float *p = (float *)get();

        ModelPtr model = Device::GetTheDevice()->NewModel();

        for (auto g : geometries)
        {
          Device::GetTheDevice()->CreateTheDatasetDeviceEquivalent(g);
          GeometryVisDPtr gv = GeometryVis::New();
          model->AddGeometry(g, gv);
        }

        for (auto v : volumes)
        {
          Device::GetTheDevice()->CreateTheDatasetDeviceEquivalent(v);
          VolumeVisDPtr vv = VolumeVis::New();
          model->AddVolume(v, vv);
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

        model->Sample(rays);

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
        }

        return false;
    }
};

WORK_CLASS_TYPE(SetupVolume)
WORK_CLASS_TYPE(IntersectMsg)
WORK_CLASS_TYPE(SampleMsg)

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

    SetupVolume::Register();
    IntersectMsg::Register();
    SampleMsg::Register();

    if (mpiRank == 0)
    {
        {
            VolumeDPtr vp = gxy::Volume::NewDistributed();
            SetupVolume v(vp);
            v.Broadcast(true, true);

            vp->Commit();

            SampleMsg s = SampleMsg(0.0, y, -1);
            s.Broadcast(true, true);
        }

        theApplication.QuitApplication();
    }

    theApplication.Wait();

    IntelDevice::FreeDevice();
}
