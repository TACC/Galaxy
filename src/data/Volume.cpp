#define LOGGING 0

#include <iostream>
#include <math.h>
#include <stdlib.h>
#include <string>
#include <sstream>
#include <fstream>
#include "Application.h"
#include "Volume.h"
// #include "Renderer.h"
#include "Datasets.h"

#include <vtkNew.h>
#include <vtkDataSetReader.h>
#include <vtkPointData.h>
#include <vtkCharArray.h>
#include <vtkClientSocket.h>
#include <vtkXMLImageDataWriter.h>

KEYED_OBJECT_TYPE(Volume)

using namespace std;

void
Volume::initialize()
{
	initialize_grid = false;
	vtkobj = NULL;
	samples = NULL;
  super::initialize();
}

void
Volume::Register()
{
	RegisterClass();
}

Volume::~Volume()
{
	if (vtkobj) vtkobj->Delete();
	if (samples) free(samples);
}

bool 
Volume::local_commit(MPI_Comm c)
{
	OSPRayObject::local_commit(c);

	OSPVolume ospv = ospNewVolume("shared_structured_volume");

  osp::vec3i counts;
  get_ghosted_local_counts(counts.x, counts.y, counts.z);

  OSPData data = ospNewData(counts.x*counts.y*counts.z, get_type() == FLOAT ? OSP_FLOAT : OSP_UCHAR, (void *)get_samples(), OSP_DATA_SHARED_BUFFER);
  ospCommit(data);

  ospSetObject(ospv, "voxelData", data);

	MPI_Barrier(c);
	sleep(GetTheApplication()->GetRank());

  ospSetVec3i(ospv, "dimensions", counts);

	MPI_Barrier(c);

  osp::vec3f origin;
  get_ghosted_local_origin(origin.x, origin.y, origin.z);
  ospSetVec3f(ospv, "gridOrigin", origin);

  osp::vec3f spacing;
  get_deltas(spacing.x, spacing.y, spacing.z);
  ospSetVec3f(ospv, "gridSpacing", spacing);

  ospSetString(ospv, "voxelType", get_type() == FLOAT ? "float" : "uchar");

	ospSetObject(ospv, "transferFunction", ospNewTransferFunction("piecewise_linear"));

	ospSetf(ospv, "samplingRate", 1.0);

	MPI_Barrier(c);
  ospCommit(ospv);
	MPI_Barrier(c);

  if (theOSPRayObject)
    ospRelease(theOSPRayObject);
  
	theOSPRayObject = ospv;

	return false;
}

void
Volume::LoadFromJSON(Value& v)
{
 	if (v.HasMember("filename"))
	{
    set_attached(false);
    Import(string(v["filename"].GetString()), NULL, 0);
	}
 	else if (v.HasMember("layout"))
	{
		set_attached(true);
 		Attach(string(v["layout"].GetString()));
	}
 	else
 	{
 		std::cerr << "json Volume has neither filename or layout spec\n";
 		exit(1);
 	}
}

void
Volume::SaveToJSON(Value& dsets, Document&  doc)
{
	Value container = Value(kObjectType);

	if (attached)
		container.AddMember("layout", Value().SetString(filename.c_str(), doc.GetAllocator()), doc.GetAllocator());
	else
		container.AddMember("filename", Value().SetString(filename.c_str(), doc.GetAllocator()), doc.GetAllocator());

	container.AddMember("type", Value().SetString("Volume", doc.GetAllocator()), doc.GetAllocator());

	dsets.PushBack(container, doc.GetAllocator());
}

static void
factor(int ijk, vec3i &factors)
{
  // If ijk is prime, 1, 1, ijk will be chosen, i+j+k == ijk+2

  if (ijk == 1)
  {
    factors.x = factors.y = factors.z = 1;
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
            factors.x = i;
            factors.y = j;
            factors.z = k;
          }
        }
      }
    }
  }
}

struct part
{
  vec3i ijk;
  vec3i counts;
  vec3i gcounts;
  vec3i offsets;
  vec3i goffsets;
};

part *
partition(int ijk, vec3i factors, vec3i grid)
{
  int ni = grid.x - 2;
  int nj = grid.y - 2;
  int nk = grid.z - 2;

  int di = ni / factors.x;
  int dj = nj / factors.y;
  int dk = nk / factors.z;

  part *parts = new part[ijk];
  part *p = parts;
  for (int k = 0; k < factors.z; k++)
    for (int j = 0; j < factors.y; j++)
      for (int i = 0; i < factors.x; i++, p++)
      {
        p->ijk.x = i;
        p->ijk.y = j;
        p->ijk.z = k;

        p->offsets.x = 1 + i*di;
        p->offsets.y = 1 + j*dj;
        p->offsets.z = 1 + k*dk;

        p->goffsets.x = p->offsets.x - 1;
        p->goffsets.y = p->offsets.y - 1;
        p->goffsets.z = p->offsets.z - 1;

        p->counts.x = 1 + ((i == (factors.x-1)) ? ni - p->offsets.x : di);
        p->counts.y = 1 + ((j == (factors.y-1)) ? nj - p->offsets.y : dj);
        p->counts.z = 1 + ((k == (factors.z-1)) ? nk - p->offsets.z : dk);

        p->gcounts.x = p->counts.x + 2;
        p->gcounts.y = p->counts.y + 2;
        p->gcounts.z = p->counts.z + 2;
      }

  return parts;
}

void
Volume::local_import(char *fname, MPI_Comm c)
{
	string type_string, data_fname;

	filename = fname;

	int rank = GetTheApplication()->GetRank();
	int size = GetTheApplication()->GetSize();

	string dir((filename.find_last_of("/") == string::npos) ? "" : filename.substr(0, filename.find_last_of("/")+1));

	ifstream in;
	in.open(filename.c_str());
	if (in.fail())
	{
		std::cerr << "unable to open volfile\n";
		exit(1);
	}

	in >> type_string;
	type = type_string == "float" ? FLOAT : UCHAR;
	
	in >> global_origin.x >> global_origin.y >> global_origin.z;
	in >> global_counts.x >> global_counts.y >> global_counts.z;
	in >> deltas.x >> deltas.y >> deltas.z;

	in >> data_fname;
	in.close();

	vec3i global_partitions;

  if (getenv("PARTITIONING"))
  {
    if (3 != sscanf(getenv("PARTITIONING"), "%d,%d,%d", &global_partitions.x, &global_partitions.y, &global_partitions.z))
    {
      std::cerr << "Illegal PARTITIONING environment variable\n";
      exit(1);
    }
    if ((global_partitions.x*global_partitions.y*global_partitions.z) != size)
    {
      std::cerr << "PARTITIONING does not multiply to current MPI size\n";
      exit(1);
    }
  }
  else
    factor(size, global_partitions);

#if 0
  if (rank == 0)
			APP_PRINT(<< "GLOBAL PARTITIONS: " << global_partitions.x << " " << global_partitions.y << " " << global_partitions.z);
#endif

  part *partitions = partition(size, global_partitions, global_counts);

  part *my_partition = partitions + rank;

  local_offset = my_partition->offsets;
  local_counts  = my_partition->counts;
  ghosted_local_offset = my_partition->goffsets;
  ghosted_local_counts = my_partition->gcounts;

	in.open(data_fname[0] == '/' ? data_fname.c_str() : (dir + data_fname).c_str(), ios::binary | ios::in);
	size_t sample_sz = ((type == FLOAT) ? 4 : 1);

#if 0
  if ((GetTheApplication()->GetSize()-1) == GetTheApplication()->GetRank())
  {
    std::cerr << "local_count:  " << local_counts.x << " " << local_counts.y << " " << local_counts.z << "\n";;
    std::cerr << "ghosted_local_count:  " << ghosted_local_counts.x << " " << ghosted_local_counts.y << " " << ghosted_local_counts.z << "\n";
    std::cerr << "local_offset:  " << local_offset.x << " " << local_offset.y << " " << local_offset.z << "\n";;
    std::cerr << "ghosted_local_offset:  " << ghosted_local_offset.x << " " << ghosted_local_offset.y << " " << ghosted_local_offset.z << "\n";
  }
#endif

	size_t row_sz = ghosted_local_counts.x * sample_sz;
	size_t tot_sz = row_sz * ghosted_local_counts.y * ghosted_local_counts.z;

	samples = (unsigned char *)malloc(tot_sz);

	string rawname = data_fname[0] == '/' ? data_fname : (dir + data_fname);
	ifstream raw;
  raw.open(rawname.c_str(), ios::in | ios::binary);

	char *dst = (char *)samples;
	for (int z = 0; z < ghosted_local_counts.z; z++)
		for (int y = 0; y < ghosted_local_counts.y; y++)
		{
			streampos src = (((ghosted_local_offset.z + z) * (global_counts.y * global_counts.x)) + 
											 ((ghosted_local_offset.y + y) * global_counts.x) + 
											   ghosted_local_offset.x) * sample_sz;

			raw.seekg(src, ios_base::beg);
			raw.read(dst, row_sz);
			dst += row_sz;
		}

	if (type == FLOAT)
	{
		float *ptr = (float *)samples;
	  local_min = local_max = *ptr++;
		for (int i = 1; i < ghosted_local_counts.x * ghosted_local_counts.y * ghosted_local_counts.z; i++, ptr++)
		{	
			if (*ptr < local_min) local_min = *ptr;
			if (*ptr > local_max) local_max = *ptr;
		}
	}
	else
	{
		unsigned char *ptr = (unsigned char *)samples;
	  local_min = local_max = *ptr++;
		for (int i = 1; i < ghosted_local_counts.x * ghosted_local_counts.y * ghosted_local_counts.z; i++, ptr++)
		{	
			if (*ptr < local_min) local_min = *ptr;
			if (*ptr > local_max) local_max = *ptr;
		}
	}

	raw.close();

	float lmin, lmax, gmin, gmax;
	get_local_minmax(lmin, lmax);

	MPI_Allreduce(&lmax, &gmax, 1, MPI_FLOAT, MPI_MAX, c);
	MPI_Allreduce(&lmin, &gmin, 1, MPI_FLOAT, MPI_MIN, c);

	set_global_minmax(gmin, gmax);

#define ijk2rank(i, j, k) ((i) + ((j) * global_partitions.x) + ((k) * global_partitions.x * global_partitions.y))

	if (getenv("MPI_TEST_RANK"))
	{
		neighbors[0] = -1;
		neighbors[1] = -1;
		neighbors[2] = -1;
		neighbors[3] = -1;
		neighbors[4] = -1;
		neighbors[5] = -1;
	}
	else
	{
		neighbors[0] = (my_partition->ijk.x > 0) ? ijk2rank(my_partition->ijk.x - 1, my_partition->ijk.y, my_partition->ijk.z) : -1;
		neighbors[1] = (my_partition->ijk.x < (global_partitions.x-1)) ? ijk2rank(my_partition->ijk.x + 1, my_partition->ijk.y, my_partition->ijk.z) : -1;
		neighbors[2] = (my_partition->ijk.y > 0) ? ijk2rank(my_partition->ijk.x, my_partition->ijk.y - 1, my_partition->ijk.z) : -1;
		neighbors[3] = (my_partition->ijk.y < (global_partitions.y-1)) ? ijk2rank(my_partition->ijk.x, my_partition->ijk.y + 1, my_partition->ijk.z) : -1;
		neighbors[4] = (my_partition->ijk.z > 0) ? ijk2rank(my_partition->ijk.x, my_partition->ijk.y, my_partition->ijk.z - 1) : -1;
		neighbors[5] = (my_partition->ijk.z < (global_partitions.z-1)) ? ijk2rank(my_partition->ijk.x, my_partition->ijk.y, my_partition->ijk.z + 1) : -1;
	}

  float go[] = {global_origin.x + deltas.x, global_origin.y + deltas.y, global_origin.z + deltas.z};
  int   gc[] = {global_counts.x - 2, global_counts.y - 2, global_counts.z - 2};
	global_box = Box(go, gc, (float *)&deltas);

  float lo[3] =
	{
    global_origin.x + (local_offset.x * deltas.x),
    global_origin.y + (local_offset.y * deltas.y),
    global_origin.z + (local_offset.z * deltas.z)
  };

  local_box = Box(lo, (int *)&local_counts, (float *)&deltas);

#if LOGGING
	APP_LOG(<< "neighbors " << neighbors[0] << " " << neighbors[1] << " " << neighbors[2] << " " << neighbors[3] << " " << neighbors[4] << " " << neighbors[5]);
#endif
}

bool
Volume::local_load_timestep(MPI_Comm c)
{
  int sz;
  char *str;
  skt->Receive((void *)&sz, sizeof(sz));
  if (sz < 0) 
  	return true;

  vtkNew<vtkCharArray> bufArray;
  bufArray->Allocate(sz);
  bufArray->SetNumberOfTuples(sz);
  void *ptr = bufArray->GetVoidPointer(0);

  skt->Receive(ptr, sz);

  vtkNew<vtkDataSetReader> reader;
  reader->ReadFromInputStringOn();
  reader->SetInputArray(bufArray.GetPointer());
  reader->Update();

  vtkobj = vtkImageData::New();
	vtkobj->ShallowCopy(vtkImageData::SafeDownCast(reader->GetOutput()));
  if (! vtkobj) 
  {
    cerr << "received something other than a vtkImageData object\n";
    exit(1);
  }

	if (! vtkobj->GetPointData())
  {
    cerr << "vtkImageData object has no point dependent data\n";
    exit(1);
  }

	vtkDataArray *array = vtkobj->GetPointData()->GetScalars();
	if (! array)
	{
		if (vtkobj->GetPointData()->GetNumberOfArrays() == 0)
		{
			cerr << "vtkImageData object has no point dependent data\n";
			exit(1);
		}
		array = vtkobj->GetPointData()->GetArray(0);

		if (array->GetNumberOfComponents() != 1)
		{
			cerr << "Currently can only handle scalar data\n";
			exit(1);
		}
		
		vtkobj->GetPointData()->SetScalars(array);
	}

	if (array->IsA("vtkFloatArray"))
		type = FLOAT;
  else if (array->IsA("vtkUnsignedCharArray"))
    type = UCHAR;
	else
  {
    cerr << "vtkImageData object has no point dependent data\n";
    exit(1);
  }

	samples = (unsigned char *)array->GetVoidPointer(0);

  double lminmax[2], gmin, gmax;
  vtkobj->GetScalarRange(lminmax);

  MPI_Allreduce(lminmax+0, &gmin, 1, MPI_DOUBLE, MPI_MIN, c);
  MPI_Allreduce(lminmax+1, &gmax, 1, MPI_DOUBLE, MPI_MAX, c);

  set_global_minmax((float)gmin, (float)gmax);

  if (initialize_grid)
  {
		initialize_grid = false;

    int rank = GetTheApplication()->GetRank();
    int size = GetTheApplication()->GetSize();

    double lo[3], go[3];
    vtkobj->GetOrigin(lo);

		MPI_Allreduce((const void *)lo, (void *)go, 3, MPI_DOUBLE, MPI_MIN, MPI_COMM_WORLD);

    global_origin.x = go[0];
    global_origin.y = go[1];
    global_origin.z = go[2];

		double spacing[3];
    vtkobj->GetSpacing(spacing);
    deltas.x = spacing[0];
    deltas.y = spacing[1];
    deltas.z = spacing[2];

    int extent[6];
    vtkobj->GetExtent(extent);

		int offset[3];

		offset[0] = (int)(((lo[0] - go[0]) / spacing[0]) + 0.5);
		offset[1] = (int)(((lo[1] - go[1]) / spacing[1]) + 0.5);
		offset[2] = (int)(((lo[2] - go[2]) / spacing[2]) + 0.5);

		extent[0] += offset[0];
		extent[1] += offset[0];
		extent[2] += offset[1];
		extent[3] += offset[1];
		extent[4] += offset[2];
		extent[5] += offset[2];

    int extents[6*size];
    MPI_Allgather((const void *)extent, 6, MPI_INT, (void *)extents, 6, MPI_INT, MPI_COMM_WORLD);

    for (int i = 0; i < 6; i++) neighbors[i] = -1;

#define LAST(axis) (extents[i*6 + 2*axis + 1] == extent[2*axis + 0])
#define NEXT(axis) (extents[i*6 + 2*axis + 0] == extent[2*axis + 1])
#define EQ(axis)   (extents[i*6 + 2*axis + 0] == extent[2*axis + 0])

    local_offset.x = extent[0];
    local_counts.x = (extent[1] - extent[0]) + 1;

    local_offset.y = extent[2];
    local_counts.y = (extent[3] - extent[2]) + 1;

    local_offset.z = extent[4];
    local_counts.z = (extent[5] - extent[4]) + 1;

    global_counts.x = global_counts.y = global_counts.z = 0;

    for (int i = 0; i < size; i++)
    {
      if ((extents[i*6 + 2*0 + 1] + 1) > global_counts.x) global_counts.x = extents[i*6 + 2*0 + 1] + 1;
      if ((extents[i*6 + 2*1 + 1] + 1) > global_counts.y) global_counts.y = extents[i*6 + 2*1 + 1] + 1;
      if ((extents[i*6 + 2*2 + 1] + 1) > global_counts.z) global_counts.z = extents[i*6 + 2*2 + 1] + 1;

      if (LAST(0) && EQ(1) && EQ(2)) neighbors[0] = i;
      if (NEXT(0) && EQ(1) && EQ(2)) neighbors[1] = i;

      if (EQ(0) && LAST(1) && EQ(2)) neighbors[2] = i;
      if (EQ(0) && NEXT(1) && EQ(2)) neighbors[3] = i;

      if (EQ(0) && EQ(1) && LAST(2)) neighbors[4] = i;
      if (EQ(0) && EQ(1) && NEXT(2)) neighbors[5] = i;
    }
  }

	ghosted_local_offset = local_offset;
	ghosted_local_counts = local_counts;

	global_box = Box((float *)&global_origin, (int *)&global_counts, (float *)&deltas);
	
  float lo[3] =
	{
    global_origin.x + (local_offset.x * deltas.x),
    global_origin.y + (local_offset.y * deltas.y),
    global_origin.z + (local_offset.z * deltas.z)
  };

  local_box = Box(lo, (int *)&local_counts, (float *)&deltas);

	return false;
}
