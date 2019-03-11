// ========================================================================== //
// Copyright (c) 2014-2019 The University of Texas at Austin.                 //
// All rights reserved.                                                       //
//                                                                            //
// Licensed under the Apache License, Version 2.0 (the "License");            //
// you may not use this file except in compliance with the License.           //
// A copy of the License is included with this software in the file LICENSE.  //
// If your copy does not contain the License, you may obtain a copy of the    //
// License at:                                                                //
//                                                                            //
//     https://www.apache.org/licenses/LICENSE-2.0                            //
//                                                                            //
// Unless required by applicable law or agreed to in writing, software        //
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT  //
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.           //
// See the License for the specific language governing permissions and        //
// limitations under the License.                                             //
//                                                                            //
// ========================================================================== //

#include <iostream>

#include "Application.h"
#include "Volume.h"

#include <vtkNew.h>
#include <vtkDataSetReader.h>
#include <vtkPointData.h>
#include <vtkCharArray.h>
#include <vtkClientSocket.h>
#include <vtkXMLImageDataWriter.h>

using namespace rapidjson;
using namespace std;

namespace gxy
{

KEYED_OBJECT_CLASS_TYPE(Volume)

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
Volume::LoadFromJSON(Value& v)
{
  int r = GetTheApplication()->GetRank();

 	if (v.HasMember("filename"))
	{
    set_attached(false);
    return Import(string(v["filename"].GetString()), NULL, 0);
	}
 	else if (v.HasMember("layout"))
	{
		set_attached(true);
 		return Attach(string(v["layout"].GetString()));
	}
 	else
 	{
 		if (r == 0) cerr << "ERROR: json Volume has neither filename or layout spec" << endl;
 		return false;
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

bool
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
		if (rank == 0) cerr << "ERROR: unable to open volfile: " << filename << endl;;
		return false;
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
      if (rank == 0) cerr << "ERROR: Illegal PARTITIONING environment variable" << endl;
      return false;
    }
    if ((global_partitions.x*global_partitions.y*global_partitions.z) != size)
    {
      if (rank == 0) cerr << "ERROR: json PARTITIONING does not multiply to current MPI size" << endl;
      return false;
    }
  }
  else
    factor(size, global_partitions);

  part *partitions = partition(size, global_partitions, global_counts);
  part *my_partition = partitions + rank;

  local_offset = my_partition->offsets;
  local_counts  = my_partition->counts;
  ghosted_local_offset = my_partition->goffsets;
  ghosted_local_counts = my_partition->gcounts;

	in.open(data_fname[0] == '/' ? data_fname.c_str() : (dir + data_fname).c_str(), ios::binary | ios::in);
	size_t sample_sz = ((type == FLOAT) ? 4 : 1);

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

	if (GetTheApplication()->GetTheMessageManager()->UsingMPI())
	{
		MPI_Allreduce(&lmax, &gmax, 1, MPI_FLOAT, MPI_MAX, c);
		MPI_Allreduce(&lmin, &gmin, 1, MPI_FLOAT, MPI_MIN, c);
	}
	else
	{
		gmin = lmin;
		gmax = lmax;
	}

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
  return true;
}

bool
Volume::local_load_timestep(MPI_Comm c)
{
  int err = 0;
  int rank = GetTheApplication()->GetRank();
  int size = GetTheApplication()->GetSize();

  int sz;
  char *str;
  skt->Receive((void *)&sz, sizeof(sz));

  // Returns <0 if socket has closed indicating sim has gone away

  int gerr;
  MPI_Allreduce(&gerr, &sz, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);

  // If all are closed...  No error
  if (gerr <= 0)
    return true;

  if (sz > 0)
  {
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
      if (rank == 0) cerr << "ERROR: received something other than a vtkImageData object" << endl;
      err = 1;
    }
    else if (! vtkobj->GetPointData())
    {
      if (rank == 0) cerr << "ERROR: vtkImageData object has no point dependent data" << endl;
      err = 2;
    }
    else
    {
      vtkDataArray *array = vtkobj->GetPointData()->GetScalars();
      if (! array)
      {
        for (auto i = 0; i < vtkobj->GetPointData()->GetNumberOfArrays(); i++)
        {
          array = vtkobj->GetPointData()->GetArray(i);
          if (array->GetNumberOfComponents() == 1)
            break;
          array = NULL;
        }
      }

      if (array == NULL)
      {
        if (rank == 0) cerr << "ERROR: Currently can only handle point-dependent scalar data" << endl;
        err = 3;
      }
      else
      {
        vtkobj->GetPointData()->SetScalars(array);

        if (array->IsA("vtkFloatArray"))
          type = FLOAT;
        else if (array->IsA("vtkUnsignedCharArray"))
          type = UCHAR;
        else
        {
          if (rank == 0) cerr << "ERROR: Currently can only handle ubyte and floay data" << endl;
          err = 3;
        }
      }

      samples = (unsigned char *)array->GetVoidPointer(0);
    }

  }

  MPI_Allreduce(&gerr, &err, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);
  if (gerr) return false;

  double lminmax[2], gmin, gmax;
  vtkobj->GetScalarRange(lminmax);

	if (GetTheApplication()->GetTheMessageManager()->UsingMPI())
	{
		MPI_Allreduce(lminmax+0, &gmin, 1, MPI_DOUBLE, MPI_MIN, c);
		MPI_Allreduce(lminmax+1, &gmax, 1, MPI_DOUBLE, MPI_MAX, c);
	}
	else
	{
		gmin = lminmax[0];
		gmax = lminmax[1];
	}

  set_global_minmax((float)gmin, (float)gmax);

  if (initialize_grid)
  {
		initialize_grid = false;


    double lo[3], go[3];
    vtkobj->GetOrigin(lo);

		if (GetTheApplication()->GetTheMessageManager()->UsingMPI())
			MPI_Allreduce((const void *)lo, (void *)go, 3, MPI_DOUBLE, MPI_MIN, MPI_COMM_WORLD);
		else
			for (int i = 0; i < 3; i++) go[i] = lo[i];

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

		local_offset.x = extent[0];
		local_counts.x = (extent[1] - extent[0]) + 1;

		local_offset.y = extent[2];
		local_counts.y = (extent[3] - extent[2]) + 1;

		local_offset.z = extent[4];
		local_counts.z = (extent[5] - extent[4]) + 1;

		if (GetTheApplication()->GetTheMessageManager()->UsingMPI())
		{
			int extents[6*size];
			MPI_Allgather((const void *)extent, 6, MPI_INT, (void *)extents, 6, MPI_INT, MPI_COMM_WORLD);
			
			for (int i = 0; i < 6; i++) neighbors[i] = -1;

#define LAST(axis) (extents[i*6 + 2*axis + 1] == extent[2*axis + 0])
#define NEXT(axis) (extents[i*6 + 2*axis + 0] == extent[2*axis + 1])
#define EQ(axis)   (extents[i*6 + 2*axis + 0] == extent[2*axis + 0])

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
		else
		{
			for (int i = 0; i < 6; i++) neighbors[i] = -1;
			global_counts = local_counts;
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

	return true;
}

} // namespace gxy
