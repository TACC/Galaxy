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
#if 0
 	else if (v.HasMember("layout"))
	{
		set_attached(true);
 		return Attach(string(v["layout"].GetString()));
	}
#endif
 	else
 	{
 		if (r == 0) cerr << "ERROR: json Volume has neither filename or layout spec" << endl;
 		return false;
 	}
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

#if 0
  if (rank == 0)
    for (int i = 0; i < size; i++)
    {
      part *p = partitions + i;
      std::cerr << i << ":\n"
                << "  ijk: " << p->ijk.x << " " << p->ijk.y << " " << p->ijk.z << "\n"
                << "  counts: " << p->counts.x << " " << p->counts.y << " " << p->counts.z << "\n"
                << "  gcounts: " << p->gcounts.x << " " << p->gcounts.y << " " << p->gcounts.z << "\n"
                << "  offsets: " << p->offsets.x << " " << p->offsets.y << " " << p->offsets.z << "\n"
                << "  goffsets: " << p->goffsets.x << " " << p->goffsets.y << " " << p->goffsets.z << "\n";
    }
#endif

  return true;
}

} // namespace gxy
