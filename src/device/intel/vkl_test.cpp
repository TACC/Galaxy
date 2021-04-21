#include <Application.h>
#include <DataObjects.h>
#include <Rays.h>

#include "Volume.h"

#include "IntelDevice.h"
#include "IntelModel.h"

#include "VklVolume.h"
#include "VklVolume_ispc.h"

#define NSAMP 10
int nsamp = NSAMP;

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

VolumeDPtr   volume;

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
              *ptr++ = (i/31.0) + 0.01*(k/31.0);
        
        Device::GetTheDevice()->CreateTheDatasetDeviceEquivalent(volume);

        return false;
    }
}; 

class IsoCrossingMsg : public Work
{
public:
    IsoCrossingMsg(int n, float *v) : IsoCrossingMsg(sizeof(float) + sizeof(int))
    {
        float *p = (float *)get();
        *(int *)p++ = n;
        for (int i = 0; i < n; i++)
          *p++ = *v++;
    }

    ~IsoCrossingMsg() {}
    WORK_CLASS(IsoCrossingMsg, true)

public:
    bool CollectiveAction(MPI_Comm c, bool isRoot)
    {   
        float *p = (float *)get();
        int nv = *(int *)p;
        float *v = p + 1;

        RayList *rays = new RayList(nsamp);

        for (int i = 0; i < nsamp; i++)
        {
            rays->set_ox(i, mpiRank + (nsamp == 1 ? 0.5 : -0.1 + (1.2/float(nsamp-1))*i));
            rays->set_oy(i, 0.5);
            rays->set_oz(i, -1.0);
            rays->set_dx(i, 0);
            rays->set_dy(i, 0);
            rays->set_dz(i, 1);
            rays->set_t(i, 0);
            //rays->set_term(i, -1);
            //rays->set_tMax(i, std::numeric_limits<float>::infinity());
            rays->set_term(i, 1);
            rays->set_tMax(i, 2);
        }

        VklVolumePtr vklv = VklVolume::Cast(volume->GetTheDeviceEquivalent());
        ::ispc::VklVolume_ispc *vklispc = (::ispc::VklVolume_ispc *)vklv->GetIspc();
        ::ispc::VklVolume_TestIsoCrossing(vklispc, nsamp, rays->GetIspc(), nv, v);

        for (int j = 0; j < mpiSize; j++)
        {
            MPI_Barrier(c);

            if (j == mpiRank)
            {
                for (int i = 0; i < nsamp; i++)
                {
                    char c;
                    switch (rays->get_term(i))
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
                
                for (int i = 0; i < nsamp; i++)
                    if (rays->get_term(i))
                    {
                        float t = rays->get_tMax(i);
                        std::cerr << i << ": " 
                            << "(" << (rays->get_ox(i) + t*(rays->get_dx(i)))
                            << " " << (rays->get_oy(i) + t*(rays->get_dy(i)))
                            << " " << (rays->get_oz(i) + t*(rays->get_dz(i)))
                            << ") " << t << "\n";
                    }
            }
        }
        return false;
    }
};

class SampleMsg : public Work
{
public:

    ~SampleMsg() {}
    WORK_CLASS(SampleMsg, true)

public:
    bool CollectiveAction(MPI_Comm c, bool isRoot)
    {   
        float x[nsamp];
        float y[nsamp];
        float z[nsamp];
        float d[nsamp];
        for (int i = 0; i < nsamp; i++)
        {
            x[i] = mpiRank + (nsamp == 1 ? 0.5 : (1/float(nsamp-1))*i);
            y[i] = 0.5;
            z[i] = (nsamp == 1 ? 0.5 : (1/float(nsamp-1))*i);
        }

        VklVolumePtr vklv = VklVolume::Cast(volume->GetTheDeviceEquivalent());
        ::ispc::VklVolume_ispc *vklispc = (::ispc::VklVolume_ispc *)vklv->GetIspc();
        ::ispc::VklVolume_TestSample(vklispc, nsamp, x, y, z, d);

        for (int j = 0; j < mpiSize; j++)
        {
            if (j == mpiRank)
              for (int i = 0; i < nsamp; i++)
                std::cerr << i << " :: (" << x[i] << ", " << y[i] << ", " << z[i] << ") " << d[i] << "\n";
            MPI_Barrier(c);
        }

        return false;
    }
};

WORK_CLASS_TYPE(SetupVolume)
WORK_CLASS_TYPE(SampleMsg)
WORK_CLASS_TYPE(IsoCrossingMsg)

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
    float v = 0.5;

    IntelDevice::InitDevice();

    Application theApplication(&argc, &argv);

    RegisterDataObjects();

    theApplication.Start();

    for (int i = 1; i < argc; i++)
        if (!strncmp(argv[i], "-D", 2)) dbg = true, atch = false, dbgarg = argv[i] + 2;
        else if (!strcmp(argv[i], "-t")) test_element_types |= atoi(argv[++i]);
        else if (!strcmp(argv[i], "-y")) y = atof(argv[++i]);
        else if (!strcmp(argv[i], "-z")) show_Z = true;
        else if (!strcmp(argv[i], "-v")) v = atof(argv[++i]);
        else if (!strcmp(argv[i], "-N")) nsamp = atoi(argv[++i]);
        else syntax(argv[0]);

    mpiRank = theApplication.GetRank();
    mpiSize = theApplication.GetSize();

    Debug *d = dbg ? new Debug(argv[0], atch, dbgarg) : NULL;

    theApplication.Run();

    SetupVolume::Register();
    SampleMsg::Register();
    IsoCrossingMsg::Register();

    if (mpiRank == 0)
    {
        {
            volume = gxy::Volume::NewDistributed();

            SetupVolume sv(volume);
            sv.Broadcast(true, true);

            volume->Commit();

            SampleMsg s = SampleMsg();
            s.Broadcast(true, true);

            IsoCrossingMsg i = IsoCrossingMsg(1, &v);
            i.Broadcast(true, true);
        }

        theApplication.QuitApplication();
    }

    theApplication.Wait();

    IntelDevice::FreeDevice();
}
