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
#include "OsprayVolume.h"

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
  number_of_components = 1;
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
	string ext((filename.find_last_of(".") == string::npos) ? "vol" : filename.substr(filename.find_last_of(".")+1));

	ifstream in;
	in.open(filename.c_str());
	if (in.fail())
	{
		if (rank == 0) cerr << "ERROR: unable to open volfile: " << filename << endl;;
		return false;
	}

  if (ext == "vol")
  {
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
  }
  else if (ext == "json")
  {
    ifstream in;
    in.open(filename.c_str());
    if (in.fail())
    {
      if (rank == 0) cerr << "ERROR: unable to open volfile: " << filename << endl;;
      return false;
    }

    stringstream ss;
    ss << in.rdbuf();

    Document doc;
    if (doc.Parse<0>(ss.str().c_str()).HasParseError())
    {
      std::cerr << "JSON parse error in " << fname << "\n";
      in.close();
      return false;
    }

    if (doc.HasMember("type")) type = (doc["type"].GetString() == string("float")) ? FLOAT : UCHAR;
    else 
    {
      std::cerr << "volume JSON has no type field: " << fname << "\n";
      return false;
      in.close();
    }

    if (doc.HasMember("origin"))
    {
      global_origin.x = doc["origin"][0].GetDouble();
      global_origin.y = doc["origin"][1].GetDouble();
      global_origin.z = doc["origin"][2].GetDouble();
    }
    else 
    {
      std::cerr << "volume JSON has no origin field: " << fname << "\n";
      return false;
    }

    if (doc.HasMember("counts"))
    {
      global_counts.x = doc["counts"][0].GetInt();
      global_counts.y = doc["counts"][1].GetInt();
      global_counts.z = doc["counts"][2].GetInt();
    }
    else 
    {
      std::cerr << "volume JSON has no counts field: " << fname << "\n";
      in.close();
      return false;
    }

    if (doc.HasMember("delta"))
    {
      deltas.x = doc["delta"][0].GetDouble();
      deltas.y = doc["delta"][1].GetDouble();
      deltas.z = doc["delta"][2].GetDouble();
    }
    else 
    {
      std::cerr << "volume JSON has no deltas field: " << fname << "\n";
      in.close();
      return false;
    }

    if (doc.HasMember("rawdata"))
      data_fname = doc["rawdata"].GetString();
    else 
    {
      std::cerr << "volume JSON has no rawdata field: " << fname << "\n";
      in.close();
      return false;
    }

    if (doc.HasMember("number of components"))
      number_of_components = doc["number of components"].GetInt();
    else 
      number_of_components = 1;

    in.close();
  }
  else
  {
    cerr << "Volume::local_import: unrecognized file extension (" << ext << ")\n";
    return false;
  }

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

  ijk = my_partition->ijk;
  //std::cerr << "XXXX size " << size << " ijk " << ijk.x << " " << ijk.y << " " << ijk.z << "\n";

  local_offset = my_partition->offsets;
  local_counts  = my_partition->counts;
  ghosted_local_offset = my_partition->goffsets;
  ghosted_local_counts = my_partition->gcounts;

	in.open(data_fname[0] == '/' ? data_fname.c_str() : (dir + data_fname).c_str(), ios::binary | ios::in);
	size_t sample_sz = number_of_components * ((type == FLOAT) ? 4 : 1);

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

	raw.close();

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
		neighbors[0] = (ijk.x > 0) ? ijk2rank(ijk.x - 1, ijk.y, ijk.z) : -1;
		neighbors[1] = (ijk.x < (global_partitions.x-1)) ? ijk2rank(ijk.x + 1, ijk.y, ijk.z) : -1;
		neighbors[2] = (ijk.y > 0) ? ijk2rank(ijk.x, ijk.y - 1, ijk.z) : -1;
		neighbors[3] = (ijk.y < (global_partitions.y-1)) ? ijk2rank(ijk.x, ijk.y + 1, ijk.z) : -1;
		neighbors[4] = (ijk.z > 0) ? ijk2rank(ijk.x, ijk.y, ijk.z - 1) : -1;
		neighbors[5] = (ijk.z < (global_partitions.z-1)) ? ijk2rank(ijk.x, ijk.y, ijk.z + 1) : -1;
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
Volume::local_commit(MPI_Comm c)
{
  if (super::local_commit(c))  
    return true;

  if (! samples)
  {
    std::cerr << "Volume commit before anything has been loaded\n";
    return true;
  }

	if (type == FLOAT)
	{
		float *ptr = (float *)samples;
    if (number_of_components == 1)
    {
      local_min = local_max = *ptr++;
      for (int i = 1; i < ghosted_local_counts.x * ghosted_local_counts.y * ghosted_local_counts.z; i++, ptr++)
      {	
        if (*ptr < local_min) local_min = *ptr;
        if (*ptr > local_max) local_max = *ptr;
      }
    }
    else
    {
      for (int i = 0; i < ghosted_local_counts.x * ghosted_local_counts.y * ghosted_local_counts.z; i++)
      {	
        double d = 0;
        for (int j = 0; j < number_of_components; j++, ptr++)
          d += ((*ptr) * (*ptr));
        if (i == 0)
          local_min = local_max = d;
        else
        {
          if (d < local_min) local_min = d;
          if (d > local_max) local_max = d;
        }
      }
      local_min = sqrt(local_min);
      local_max = sqrt(local_max);
    }
	}
	else
	{
		unsigned char *ptr = (unsigned char *)samples;
    if (number_of_components == 1)
    {
      local_min = local_max = *ptr++;
      for (int i = 1; i < ghosted_local_counts.x * ghosted_local_counts.y * ghosted_local_counts.z; i++, ptr++)
      {	
        if (*ptr < local_min) local_min = *ptr;
        if (*ptr > local_max) local_max = *ptr;
      }
    }
    else
    {
      for (int i = 0; i < ghosted_local_counts.x * ghosted_local_counts.y * ghosted_local_counts.z; i++)
      {	
        double d = 0;
        for (int j = 0; j < number_of_components; j++, ptr++)
        {
          double p = *ptr;
          d += (p * p);
        }
        if (i == 0)
          local_min = local_max = d;
        else
        {
          if (d < local_min) local_min = d;
          if (d > local_max) local_max = d;
        }
      }
      local_min = sqrt(local_min);
      local_max = sqrt(local_max);
    }
	}


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

  return false;
}

#define get_sample_ptr(ijk)                                               \
  (samples + ((ijk.z * ghosted_local_counts.y * ghosted_local_counts.x)   \
              +  (ijk.y * ghosted_local_counts.x)                         \
              +  ijk.x) * number_of_components * (isFloat() ? 4 : 1))

#define interp1(T, t, a, b, d)                      \
  float t[number_of_components];                    \
  {                                                 \
    T *aptr = (T *)a, *bptr = (T *)b;               \
    for (int i = 0; i < number_of_components; i++)  \
      t[i] = (1-d)*(aptr[i]) + d*(bptr[i]);         \
  }
  
#define interp2(t, aptr, bptr, d)                   \
  float t[number_of_components];                    \
  {                                                 \
    for (int i = 0; i < number_of_components; i++)  \
      t[i] = (1-d)*(aptr[i]) + d*(bptr[i]);         \
  }
  
#define interp3(aptr, bptr, d)                      \
  {                                                 \
    for (int i = 0; i < number_of_components; i++)  \
      result[i] = (1-d)*(aptr[i]) + d*(bptr[i]);    \
  }
  

vec3i add(vec3i a, vec3i b) { return vec3i(a.x + b.x, a.y + b.y, a.z + b.z); }

bool
Volume::Sample(vec3f& p, vec3f& v)
{
  float values[number_of_components];
  if (! Sample(p, values))
    return false;

  v.x = values[0];
  v.y = number_of_components > 1 ? values[1] : 0.0;
  v.z = number_of_components > 2 ? values[2] : 0.0;

  return true;
}

bool
Volume::Sample(vec3f& p, float& v)
{
  float values[number_of_components];
  if (! Sample(p, values))
    return false;

  v = values[0];
  return true;
}

bool
Volume::Sample(vec3f& p, float* result)
{
  // Get p in grid coordinates
  vec3f grid_xyz = (p - global_origin);
  grid_xyz.x = grid_xyz.x / deltas.x;
  grid_xyz.y = grid_xyz.y / deltas.y;
  grid_xyz.z = grid_xyz.z / deltas.z;

  // Lower corner of containing hex
  vec3i ll(floor(grid_xyz.x), floor(grid_xyz.y), floor(grid_xyz.z));

  if (ll.x < ghosted_local_offset.x || ll.x >= (ghosted_local_offset.x + (ghosted_local_counts.x-1))) return false;
  if (ll.y < ghosted_local_offset.y || ll.y >= (ghosted_local_offset.y + (ghosted_local_counts.y-1))) return false;
  if (ll.z < ghosted_local_offset.z || ll.z >= (ghosted_local_offset.z + (ghosted_local_counts.z-1))) return false;

  float dx = grid_xyz.x - ll.x;
  float dy = grid_xyz.y - ll.y;
  float dz = grid_xyz.z - ll.z;

  ll = ll - ghosted_local_offset;

  if (isFloat())
  {
    interp1(float, t00, get_sample_ptr(add(ll, vec3i(0, 0, 0))), get_sample_ptr(add(ll, vec3i(1, 0, 0))), dx);
    interp1(float, t01, get_sample_ptr(add(ll, vec3i(0, 0, 1))), get_sample_ptr(add(ll, vec3i(1, 0, 1))), dx);
    interp1(float, t10, get_sample_ptr(add(ll, vec3i(0, 1, 0))), get_sample_ptr(add(ll, vec3i(1, 1, 0))), dx);
    interp1(float, t11, get_sample_ptr(add(ll, vec3i(0, 1, 1))), get_sample_ptr(add(ll, vec3i(1, 1, 1))), dx);
    interp2(t0, t00, t10, dy);
    interp2(t1, t01, t11, dy);
    interp3(t0, t1, dz);
  }
  else
  {
    interp1(unsigned char, t00, get_sample_ptr(add(ll, vec3i(0, 0, 0))), get_sample_ptr(add(ll, vec3i(1, 0, 0))), dx);
    interp1(unsigned char, t01, get_sample_ptr(add(ll, vec3i(0, 0, 1))), get_sample_ptr(add(ll, vec3i(1, 0, 1))), dx);
    interp1(unsigned char, t10, get_sample_ptr(add(ll, vec3i(0, 1, 0))), get_sample_ptr(add(ll, vec3i(1, 1, 0))), dx);
    interp1(unsigned char, t11, get_sample_ptr(add(ll, vec3i(0, 1, 1))), get_sample_ptr(add(ll, vec3i(1, 1, 1))), dx);
    interp2(t0, t00, t10, dy);
    interp2(t1, t01, t11, dy);
    interp3(t0, t1, dz);
  }

  return true;
}

int 
Volume::PointOwner(vec3f& p)
{
  // Transform p to real grid space

  vec3f gp = p - global_origin;
  gp.x = gp.x / deltas.x;
  gp.y = gp.y / deltas.y;
  gp.z = gp.z / deltas.z;

  // lower corner of containing cell in global grid

  vec3i ll(floor(gp.x), floor(gp.y), floor(gp.z));

  // Is that in non-ghosted region?
  if ((ll.x < 1) || (ll.x >= (global_counts.x - 2)) ||(ll.y < 1) || (ll.y >= (global_counts.y - 2)) ||(ll.z < 1) || (ll.z >= (global_counts.z - 2))) return -1;

  int nx = (global_counts.x - 2) / global_partitions.x;
  int ny = (global_counts.y - 2) / global_partitions.y;
  int nz = (global_counts.z - 2) / global_partitions.z;

  int I = (ll.x - 1) / nx; if (I >= global_partitions.x) I = global_partitions.x - 1;
  int J = (ll.y - 1) / ny; if (J >= global_partitions.y) J = global_partitions.y - 1;
  int K = (ll.z - 1) / nz; if (K >= global_partitions.z) K = global_partitions.z - 1;

  return K*(global_partitions.x * global_partitions.y) + (J * global_partitions.x) + I;
}

OsprayObjectP 
Volume::CreateTheOSPRayEquivalent(KeyedDataObjectP kdop)
{
  if (! ospData || hasBeenModified())
  {
    ospData = OsprayObject::Cast(OsprayVolume::NewP(Volume::Cast(kdop)));
    setModified(false);
  }

  return ospData;
}
 
} // namespace gxy
