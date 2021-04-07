// ========================================================================== //
// Copyright (c) 2014-2020 The University of Texas at Austin.                 //
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
#include "Partitioning.h"
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
Volume::LoadFromJSON(PartitioningP p, Value& v)
{
  int r = GetTheApplication()->GetRank();

 	if (v.HasMember("filename"))
	{
    set_attached(false);
    return Import(p, string(v["filename"].GetString()), NULL, 0);
	}
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
Volume::local_import(PartitioningP partitioning, char *fname, MPI_Comm c)
{
	string type_string, data_fname;

	filename = fname;

	int rank = GetTheApplication()->GetRank();
	int size = GetTheApplication()->GetSize();

  Box local_box;
  partitioning->get_local_box(local_box);

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

  empty = true;

  // X range in index space that we'll need to load
  // lower index is floor

  int lx = int((local_box.xyz_min.x - global_origin.x) / deltas.x);
  if (lx >= global_counts.x) 
      return true;
  if (lx > 0) lx = lx - 1; // Add a ghost zone

  // upper index is ceiling

  int ux = int((local_box.xyz_max.x - global_origin.x) / deltas.x) + 1;
  if (ux <= 0)
      return true;
  if (ux < (global_counts.x - 1)) ux = ux + 1; // Add a ghost zone

  // Y range in index space that we'll need to load
  // lower index is floor

  int ly = int((local_box.xyz_min.y - global_origin.y) / deltas.y);
  if (ly >= global_counts.y) 
      return true;
  if (ly > 0) ly = ly - 1; // Add a ghost zone

  // upper index is ceiling

  int uy = int((local_box.xyz_max.y - global_origin.y) / deltas.y) + 1;
  if (uy <= 0)
      return true;
  if (uy < (global_counts.y - 1)) uy = uy + 1; // Add a ghost zone

  // Z range in index space that we'll need to load
  // lower index is floor

  int lz = int((local_box.xyz_min.z - global_origin.z) / deltas.z);
  if (lz >= global_counts.z) 
      return true;
  if (lz > 0) lz = lz - 1; // Add a ghost zone

  // upper index is ceiling

  int uz = int((local_box.xyz_max.z - global_origin.z) / deltas.z) + 1;
  if (uz <= 0)
      return true;
  if (uz < (global_counts.z - 1)) uz = uz + 1; // Add a ghost zone

  local_offset = vec3i(lx, ly, lz);
  local_counts = vec3i((ux - lx) + 1, (uy - ly) + 1, (uz - lz) + 1);

	in.open(data_fname[0] == '/' ? data_fname.c_str() : (dir + data_fname).c_str(), ios::binary | ios::in);
	size_t sample_sz = number_of_components * ((type == FLOAT) ? 4 : 1);

	size_t row_sz = local_counts.x * sample_sz;
	size_t tot_sz = row_sz * local_counts.y * local_counts.z;

	samples = (unsigned char *)malloc(tot_sz);

	string rawname = data_fname[0] == '/' ? data_fname : (dir + data_fname);
	ifstream raw;
  raw.open(rawname.c_str(), ios::in | ios::binary);

	char *dst = (char *)samples;
	for (int z = 0; z < local_counts.z; z++)
		for (int y = 0; y < local_counts.y; y++)
		{
			streampos src = (((local_offset.z + z) * (global_counts.y * global_counts.x)) + 
											 ((local_offset.y + y) * global_counts.x) + 
											   local_offset.x) * sample_sz;

			raw.seekg(src, ios_base::beg);
			raw.read(dst, row_sz);
			dst += row_sz;
		}

	raw.close();

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
      for (int i = 1; i < local_counts.x * local_counts.y * local_counts.z; i++, ptr++)
      {	
        if (*ptr < local_min) local_min = *ptr;
        if (*ptr > local_max) local_max = *ptr;
      }
    }
    else
    {
      for (int i = 0; i < local_counts.x * local_counts.y * local_counts.z; i++)
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
      for (int i = 1; i < local_counts.x * local_counts.y * local_counts.z; i++, ptr++)
      {	
        if (*ptr < local_min) local_min = *ptr;
        if (*ptr > local_max) local_max = *ptr;
      }
    }
    else
    {
      for (int i = 0; i < local_counts.x * local_counts.y * local_counts.z; i++)
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
  (samples + ((ijk.z * local_counts.y * local_counts.x)   \
              +  (ijk.y * local_counts.x)                         \
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

  if (ll.x < local_offset.x || ll.x >= (local_offset.x + (local_counts.x-1))) return false;
  if (ll.y < local_offset.y || ll.y >= (local_offset.y + (local_counts.y-1))) return false;
  if (ll.z < local_offset.z || ll.z >= (local_offset.z + (local_counts.z-1))) return false;

  float dx = grid_xyz.x - ll.x;
  float dy = grid_xyz.y - ll.y;
  float dz = grid_xyz.z - ll.z;

  ll = ll - local_offset;

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

OsprayObjectP 
Volume::CreateTheOSPRayEquivalent(KeyedDataObjectP kdop)
{
  OsprayVolumeP op = OsprayVolume::NewL();
  op->SetVolume(Volume::Cast(kdop));
  ospData = OsprayObject::Cast(op);
  return ospData;
}
 
} // namespace gxy
