#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <mpi.h>
#include <vector>
#include <sys/stat.h>

#include "dtypes.h"

using namespace std;
using namespace pvol;

#include <vtkImageData.h>
#include <vtkUnsignedCharArray.h>
#include <vtkFloatArray.h>
#include <vtkPointData.h>
#include <vtkCellData.h>
#include <vtkContourFilter.h>

#include <vtkXMLImageDataWriter.h>
#include <vtkXMLPolyDataWriter.h>

#include <vtkRemoveGhosts.h>

int mpi_rank, mpi_size;

static void
factor(int ijk, vec3i& f)
{
  if (ijk == 1)
  {
    f.x = f.y = f.z = 1;
    return;
  }

  int mm = ijk+3;
  for (int i = 1; i <= ijk>>1; i++)
  {
    int jk = ijk / i;
    if (ijk == (i * jk))
    {
      for (int j = 1; j <= jk>>1; j++)
      {
        int k = jk / j;
        if (jk == (j * k))
        {
          int m = i + j + k;
          if (m < mm)
          {
            mm = m;
            f.x = i;
            f.y = j;
            f.z = k;
          }
        }
      }
    }
  }
}

struct grid
{
  vec3i  ijk;
  vec3i  count;
  vec3i  gcount;
  vec3i  offset;
  vec3i  goffset;
  vec3f  origin;
  vec3f  stepsize;
  bool   isFloat;
};

grid *
partition(int ijk, vec3i factors, grid g)
{
  // Suppose the number of samples along an input axis (n) is 7 and the
  // number of partitions (p) is 2: target count per part (tcpp) is:
  //
  // tcpp = floor(n/p) + 1 = floor(7/2) + 1 = 4 
  //
  // 0 1 2 3 4 5 6 
  // x x x x x x x
  // |     |          offsets are i * (tcpp - 1) ... 0, 3
  //    4      4      counts are all tcpp ... 4, 4
  //
  // Now suppose 8 rather than 7... tcpp is 
  //
  // floor(ni/np) + 1 = floor(8/2) + 1 = 5 
  //
  // 0 1 2 3 4 5 6 7
  // x x x x x x x x
  // |       |        offsets are still i * (tcpp - 1) ... 0, 4 - 1) ... 0, 4
  //     5      4      counts are tcpp EXCEPT LAST which is ni - offset = (8 - 4) = 4
  //

  // Number of samples in the input grid ... all are REAL

  int ni = g.count.x;            // total count (n) along each axis
  int nj = g.count.y;
  int nk = g.count.z;

  int di = (ni / factors.x) + 1;  // factors is the number of partitions along
  int dj = (nj / factors.y) + 1;  // each axis, so this is tcpp along each axis
  int dk = (nk / factors.z) + 1;

  grid *partitions = new grid[ijk];
  grid *p = partitions;
  for (int k = 0; k < factors.z; k++)
    for (int j = 0; j < factors.y; j++)
      for (int i = 0; i < factors.x; i++, p++)
      {
        p->ijk.x = i;  // indices of partition along each axis
        p->ijk.y = j;
        p->ijk.z = k;

        // Offset of first REAL sample in the partition

        p->offset.x = i*(di - 1);
        p->offset.y = j*(dj - 1);
        p->offset.z = k*(dk - 1);

        p->count.x = (i == (factors.x-1)) ? (ni - p->offset.x) : di;
        p->count.y = (j == (factors.y-1)) ? (nj - p->offset.y) : dj;
        p->count.z = (k == (factors.z-1)) ? (nk - p->offset.z) : dk;

        // ghosted offsets are one less than the real offsets, except
        // for the first
        
        p->goffset.x = p->offset.x - ((i == 0) ? 0 : 1);
        p->goffset.y = p->offset.y - ((j == 0) ? 0 : 1);
        p->goffset.z = p->offset.z - ((k == 0) ? 0 : 1);
        
        // ghosted counts are +1 if not on the lower boundary and +1 if 
        // not on upper boundary

        p->gcount.x = p->count.x + ((i == 0) ? 0 : 1) + ((i == (factors.x-1)) ? 0 : 1);
        p->gcount.y = p->count.y + ((j == 0) ? 0 : 1) + ((j == (factors.y-1)) ? 0 : 1);
        p->gcount.z = p->count.z + ((k == 0) ? 0 : 1) + ((k == (factors.z-1)) ? 0 : 1);

        p->origin   = g.origin;    
        p->stepsize = g.stepsize;
        p->isFloat  = g.isFloat;
      }

  return partitions;
}

grid
get_global_grid(string vname, string& dname)
{
  grid g;
  
  ifstream in;
  in.open(vname.c_str());
  if (in.fail())
  {
    std::cerr << "unable to open volfile\n";
    exit(1);

  }

  string type_string;
  in >> type_string;
  g.isFloat = type_string == "float";

  in >> g.origin.x >> g.origin.y >> g.origin.z;
  in >> g.count.x >> g.count.y >> g.count.z;
  in >> g.stepsize.x >> g.stepsize.y >> g.stepsize.z;

  g.gcount = g.count;

  g.offset = vec3i(0, 0, 0);
  g.goffset = g.offset;

  in >> dname;
  in.close();

  return g;
}

void
syntax(char *a)
{
  if (mpi_rank == 0)
  {
    std::cerr << "syntax: [-n np | -p nx ny nz] [options] volname\n";
    std::cerr << "options:\n";
    std::cerr << "  -c contourvalue            at least 1\n";
  }
  MPI_Finalize();
  exit(1);
}

void
read_partition_data(grid p, grid g, string fname, void* data)
{
  size_t ssz = g.isFloat ? sizeof(float) : sizeof(unsigned char);
  size_t tsz = p.gcount.x * p.gcount.y * p.gcount.z * ssz;

  ifstream raw;
  raw.open(fname.c_str(), ios::in | ios::binary);

  size_t rsz = p.gcount.x * ssz;

  char *dst = (char *)data;

  for (int z = 0; z < p.gcount.z; z++)
    for (int y = 0; y < p.gcount.y; y++)
    {
      streampos src = (((p.goffset.z + z) * (g.gcount.y * g.gcount.x)) +
                       ((p.goffset.y + y) * g.gcount.x) +
                         p.goffset.x) * ssz;

      raw.seekg(src, ios_base::beg);
      raw.read(dst, rsz);
      dst += rsz;
    }
}

int
main(int argc, char **argv)
{
  vector<float> contours;
  string vname = "";

  MPI_Init(&argc, &argv);

  MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
  MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);
	std::cerr << mpi_rank << " ";

  int np = mpi_size;

  vec3i factors;
  factor(np, factors);

  for (int i = 1; i < argc; i++)
  {
    if (argv[i][0] == '-')
      switch(argv[i][1])
      {
        case 'c': 
          contours.push_back(atof(argv[++i]));
          break;

        case 'n': 
          np = atoi(argv[++i]);
          factor(np, factors);
          break;
          
        case 'p':
          factors.x = atoi(argv[++i]);
          factors.y = atoi(argv[++i]);
          factors.z = atoi(argv[++i]);
          np = factors.x * factors.y * factors.z;
          break;

        default:
          syntax(argv[0]);
      }
    else
    {
      if (vname == "")
        vname = argv[i];
      else
        syntax(argv[0]);
    }
  }

  if (vname == "" || factors.x == -1 || contours.size() == 0)
    syntax(argv[0]);

  string dir, fname;
  if (vname.find_last_of("/") != vname.npos)
  {
    dir   = vname.substr(0, vname.find_last_of("/"));
    fname = vname.substr(vname.find_last_of("/") + 1); 
  }
  else
  {
    dir = ".";
    fname = vname;
  }

  string base = fname.substr(0, fname.find_last_of("."));
  string ext  = fname.substr(fname.find_last_of(".") + 1);

  int err = 0;
  if (mpi_rank == 0)
  {
    if (mkdir(base.c_str(), 0755))
      if (errno != EEXIST)
      {
        std::cerr << "unable to create partition directory\n";
        err = 1;
      }
  }

	MPI_Bcast(&err, sizeof(err), MPI_CHAR, 0, MPI_COMM_WORLD);
  if (err)
  {
    MPI_Finalize();
    exit(1);
  }
    
  string dname;
  grid  global_grid  = get_global_grid(vname, dname);

  grid* partitions = partition(np, factors, global_grid);

  if (mpi_rank == 0)
  {
    ofstream ofs;
    ofs.open(base + ".ptri", ios::out);
    for (int pnum = 0; pnum < np; pnum++)
    {
      grid* p = partitions + pnum;
      ofs << p->origin.x + (p->offset.x * p->stepsize.x) << " " 
          << p->origin.y + (p->offset.y * p->stepsize.y) << " " 
          << p->origin.z + (p->offset.z * p->stepsize.z) << " " 
					<< p->origin.x + ((((p->offset.x + p->count.x) - 1) * p->stepsize.x)) << " " 
					<< p->origin.y + ((((p->offset.y + p->count.y) - 1) * p->stepsize.y)) << " "
					<< p->origin.z + ((((p->offset.z + p->count.z) - 1) * p->stepsize.z)) << " " 
          << base << "/" << "part-" << pnum << ".vtp\n";
    };

    ofs.close();
  }

  for (int pnum = mpi_rank; pnum < np; pnum += mpi_size)
  {
		std::cerr << pnum << "\n";
    grid* p = partitions + pnum;
    
#if 0
    cerr << "===========\n";
    cerr << "ijk " << p->ijk.x << " " << p->ijk.y << " " << p->ijk.z << "\n";
    cerr << "count " << p->count.x << " " << p->count.y << " " << p->count.z << "\n";
    cerr << "gcount " << p->gcount.x << " " << p->gcount.y << " " << p->gcount.z << "\n";
    cerr << "offset " << p->offset.x << " " << p->offset.y << " " << p->offset.z << "\n";
    cerr << "goffset " << p->goffset.x << " " << p->goffset.y << " " << p->goffset.z << "\n";
    cerr << "origin " << p->origin.x << " " << p->origin.y << " " << p->origin.z << "\n";
    cerr << "stepsize " << p->stepsize.x << " " << p->stepsize.y << " " << p->stepsize.z << "\n";
    cerr << "isFloat " << p->isFloat << "\n";
#endif

    vtkImageData* vgrid = vtkImageData::New();

    float box[6] = {
      p->offset.x * p->stepsize.x, ((p->offset.x + p->count.x) - 1) * p->stepsize.x,
      p->offset.y * p->stepsize.y, ((p->offset.y + p->count.y) - 1) * p->stepsize.y,
      p->offset.z * p->stepsize.z, ((p->offset.z + p->count.z) - 1) * p->stepsize.z
    };

    vgrid->SetOrigin(p->origin.x, p->origin.y, p->origin.z);
    vgrid->SetSpacing(p->stepsize.x, p->stepsize.y, p->stepsize.z);
    vgrid->SetExtent(p->goffset.x, (p->goffset.x + p->gcount.x) - 1,
                     p->goffset.y, (p->goffset.y + p->gcount.y) - 1,
                     p->goffset.z, (p->goffset.z + p->gcount.z) - 1);

    vtkDataArray *data = (p->isFloat) ? 
                    ((vtkDataArray *)vtkFloatArray::New()) : 
                    ((vtkDataArray *)vtkUnsignedCharArray::New());

    int npts = p->gcount.x * p->gcount.y * p->gcount.z;
    data->SetNumberOfComponents(1);
    data->SetNumberOfTuples(npts);
    data->SetName("scalars");

    read_partition_data(*p, global_grid, dname, data->GetVoidPointer(0));
    
    vgrid->GetPointData()->SetScalars(data);
    data->Delete();

    int not_ghosts[] = {p->offset.x, (p->offset.x + p->count.x) - 1,
                        p->offset.y, (p->offset.y + p->count.y) - 1,
                        p->offset.z, (p->offset.z + p->count.z) - 1};

    vgrid->GenerateGhostArray(not_ghosts);
    
    vtkContourFilter *cf = vtkContourFilter::New();
    cf->SetInputData(vgrid);

    cf->SetNumberOfContours(contours.size());
    for (int j = 0; j < contours.size(); j++)
      cf->SetValue(j, contours[j]);

    cf->Update(); 

    vtkPolyData* contour = cf->GetOutput();

    contour->RemoveGhostCells();

    stringstream ss;
    ss << dir + "/" + base + "/part-" << pnum << ".vtp";
		cerr << mpi_rank << " working on  " << ss.str().c_str() << "\n";

    vtkXMLPolyDataWriter *pdw = vtkXMLPolyDataWriter::New();
    pdw->SetFileName(ss.str().c_str());
    pdw->SetInputData(contour);
    pdw->Write();

#if 0
    vtkXMLImageDataWriter *idw = vtkXMLImageDataWriter::New();
    idw->SetFileName((ss.str() + ".vti").c_str());
    idw->SetInputData(vgrid);
    idw->Write();
    idw->Delete();
#endif

    vgrid->Delete();
    cf->Delete();
    pdw->Delete();
  }

  MPI_Finalize();
}
