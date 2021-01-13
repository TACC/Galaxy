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

#include <string>
#include <string.h>
#include <memory.h>
#include <vector>
#include <sys/stat.h>
#include <fcntl.h>

#include "Geometry.h"
#include "Particles.h"
#include "Triangles.h"
#include "PathLines.h"

#include "rapidjson/filereadstream.h"

using namespace std;
using namespace rapidjson;

#include <vtkerror.h>
#include <vtkSmartPointer.h>
#include <vtkXMLUnstructuredGridReader.h>
#include <vtkUnstructuredGridReader.h>
#include <vtkDataSetReader.h>
#include <vtkPointSet.h>
#include <vtkCellType.h>
#include <vtkCellIterator.h>
#include <vtkFloatArray.h>
#include <vtkDoubleArray.h>
#include <vtkPointData.h>
#include <vtkCharArray.h>
#include <vtkClientSocket.h>


namespace gxy
{
KEYED_OBJECT_CLASS_TYPE(Geometry)

void
Geometry::Register()
{
	RegisterClass();
	Particles::Register();
	Triangles::Register();
	PathLines::Register();
}

void
Geometry::initialize()
{
  vertices = std::shared_ptr< std::vector<vec3f> >(new std::vector<vec3f>);
  normals = std::shared_ptr< std::vector<vec3f> >(new std::vector<vec3f>);
  data = std::shared_ptr< std::vector<float> >(new std::vector<float>);
  connectivity = std::shared_ptr< std::vector<int> >(new std::vector<int>);

  vtkobj = nullptr;
  super::initialize();
  pthread_mutex_init(&lock, NULL);
}

bool
Geometry::local_copy(KeyedDataObjectDPtr src)
{
  if (! super::local_copy(src))
    return false;

  GeometryDPtr g = DCast(src);

  vertices             = g->vertices;
  data                 = g->data;
  connectivity         = g->connectivity;
  global_vertex_count  = g->global_vertex_count;
  global_element_count = g->global_element_count;

  return true;
}

void
Geometry::allocate_vertices(int nv)
{
  vertices->resize(nv);
  data->resize(nv);
}

void
Geometry::allocate_connectivity(int nc)
{
  connectivity->resize(nc);
}

bool
Geometry::get_partitioning_from_file(char *s)
{
  ifstream ifs(s);
  if (! ifs)
  {
    std::cerr << "Unable to open " << s << "\n";
    set_error(1);
    return false;
  }
  else
  {
    stringstream ss;
    ss << ifs.rdbuf();

    Document doc;
    if (doc.Parse<0>(ss.str().c_str()).HasParseError())
    {
      std::cerr << "JSON parse error in " << s << "\n";
      set_error(1);
      return false;
    }

    return get_partitioning(doc);
  }
}

bool
Geometry::Import(string s)
{
  struct stat info;
  if (stat(s.c_str(), &info))
  {
    cerr << "ERROR: Cannot open file " << s << endl;
    set_error(1);
    return false;
  }

  int fd = open(s.c_str(), O_RDONLY);

  char *buf = new char[info.st_size + 1];

  char *p = buf;
  for (int m, n = info.st_size; n > 0; n -= m, p += m) m = read(fd, p, n);
  *p = 0;

  close(fd);

  set_attached(false);

  bool r = KeyedDataObject::Import(buf, (void *)s.c_str(), s.size()+1);

  return r;
}

bool
Geometry::local_commit(MPI_Comm c)
{
  if (super::local_commit(c))
    return true;

  if (data->size() == 0)
    local_min = local_max = 0;
  else
  {
    float *dptr = data->data();
    local_min = local_max = *dptr ++;

    for (auto i = 1; i < data->size(); i++)
    {
      float d = *dptr++;
      if (local_min > d) local_min = d;
      if (local_max < d) local_max = d;
    }
  }

  MPI_Allreduce(&local_min, &global_min, 1, MPI_FLOAT, MPI_MIN, c);
  MPI_Allreduce(&local_max, &global_max, 1, MPI_FLOAT, MPI_MAX, c);

  local_vertex_count = vertices->size();
  local_element_count = connectivity->size();

  int local_counts[2] = {local_vertex_count, local_element_count};
  int global_counts[2];

  MPI_Allreduce(local_counts, global_counts, 2, MPI_INT, MPI_SUM, c);

  global_vertex_count = global_counts[0];
  global_element_count = global_counts[0];

  return false;
}

bool
Geometry::local_import(char *p, MPI_Comm c)
{
  vtkSmartPointer<vtkPointSet> pset;

  int rank = GetTheApplication()->GetRank();

  string json(p);
  string filename(p + strlen(p) + 1);
  string dir = (filename.find_last_of("/") == filename.npos) ? "./" : filename.substr(0, filename.find_last_of("/") + 1);
      
  Document doc;
  if (doc.Parse<0>(json.c_str()).HasParseError() || !get_partitioning(doc))
  {   
    if (rank == 0) std::cerr << "parse error in partition document: " << json << "\n";
    set_error(1);
    return false;
  }   
    
  // Checks in get_partitioning ensures this is OK
  Value& v = doc["parts"][GetTheApplication()->GetRank()];
  
  if (v.HasMember("filename"))
  {
    string fname(doc["parts"][GetTheApplication()->GetRank()]["filename"].GetString());
    string ext(fname.substr(fname.size() - 3));

    if (fname.c_str()[0] != '/')
      fname = dir + fname;

    vtkNew<VTKError> e;
    vtkNew<vtkXMLUnstructuredGridReader> rdr;
    e->watch(rdr);

    rdr->SetFileName(fname.c_str());
    rdr->Update();

    if (e->GetError())
    {
      cerr << "error reading " << fname << "\n";
      exit(1);
    }

    pset = (vtkPointSet *)(rdr->GetOutputAsDataSet());

  }
  else if (v.HasMember("port") && v.HasMember("host"))
  {
    vtkNew<vtkClientSocket> skt;

    string host(v["host"].GetString());
    int    port = v["port"].GetInt();

    if (skt->ConnectToServer(host.c_str(), port))
    {
      cerr << "error connecting to " << host << ":" << port << "\n";
      exit(1);
    }

    int sz;
    char *str;
    skt->Receive((void *)&sz, sizeof(sz));

    vtkNew<vtkCharArray> bufArray;
    bufArray->Allocate(sz);
    bufArray->SetNumberOfTuples(sz);
    void *ptr = bufArray->GetVoidPointer(0);

    skt->Receive(ptr, sz);

    vtkNew<vtkUnstructuredGridReader> rdr;
    rdr->ReadFromInputStringOn();
    rdr->SetInputArray(bufArray.GetPointer());
    rdr->Update();

    pset = (vtkPointSet *)(rdr->GetOutput());
  }

  bool r = load_from_vtkPointSet(pset);

  return r;
}

int
Geometry::serialSize()
{
  return super::serialSize();
}

unsigned char*
Geometry::serialize(unsigned char *ptr)
{
  ptr = super::serialize(ptr);
  return ptr;
}

unsigned char*
Geometry::deserialize(unsigned char *ptr)
{
  ptr = super::deserialize(ptr);
  return ptr;
}

bool
Geometry::get_partitioning(Value& doc)
{
  if (!doc.HasMember("parts"))
  {
    cerr << "partition document does not have parts\n";
    set_error(1);
    return false;
  }

  Value& parts = doc["parts"];

  if (!parts.IsArray() || parts.Size() != GetTheApplication()->GetSize())
  {
    cerr << "invalid partition document\n";
    set_error(1);
    return false;
  }

  float extents[6*parts.Size()];

  for (int i = 0; i < parts.Size(); i++)
  {
    Value& ext = parts[i]["extent"];
    for (int j = 0; j < 6; j++)
      extents[i*6 + j] = ext[j].GetDouble();
  }

  for (int i = 0; i < 6; i++) neighbors[i] = -1;

  float *extent = extents + 6*GetTheApplication()->GetRank();
	float gminmax[6], lminmax[6];

  gminmax[0] =  FLT_MAX;
  gminmax[1] = -FLT_MAX;
  gminmax[2] =  FLT_MAX;
  gminmax[3] = -FLT_MAX;
  gminmax[4] =  FLT_MAX;
  gminmax[5] = -FLT_MAX;

  memcpy(lminmax, extents + 6*GetTheApplication()->GetRank(), 6*sizeof(float));

  for (int i = 0; i < parts.Size(); i++)
  {
    float *e = extents + i*6;

#define LAST(axis) (e[2*axis + 1] == lminmax[2*axis + 0])
#define NEXT(axis) (e[2*axis + 0] == lminmax[2*axis + 1])
#define EQ(axis)   (e[2*axis + 0] == lminmax[2*axis + 0])

    if (e[0] < gminmax[0]) gminmax[0] = e[0];
    if (e[1] > gminmax[1]) gminmax[1] = e[1];
    if (e[2] < gminmax[2]) gminmax[2] = e[2];
    if (e[3] > gminmax[3]) gminmax[3] = e[3];
    if (e[4] < gminmax[4]) gminmax[4] = e[4];
    if (e[5] > gminmax[5]) gminmax[5] = e[5];

    if (LAST(0) && EQ(1) && EQ(2)) neighbors[0] = i;
    if (NEXT(0) && EQ(1) && EQ(2)) neighbors[1] = i;

    if (EQ(0) && LAST(1) && EQ(2)) neighbors[2] = i;
    if (EQ(0) && NEXT(1) && EQ(2)) neighbors[3] = i;

    if (EQ(0) && EQ(1) && LAST(2)) neighbors[4] = i;
    if (EQ(0) && EQ(1) && NEXT(2)) neighbors[5] = i;
  }

	global_box = Box(vec3f(gminmax[0], gminmax[2], gminmax[4]), vec3f(gminmax[1], gminmax[3], gminmax[5]));
	local_box = Box(vec3f(lminmax[0], lminmax[2], lminmax[4]), vec3f(lminmax[1], lminmax[3], lminmax[5]));

  return true;
}

bool
Geometry::LoadFromJSON(Value& v)
{
  if (v.HasMember("color")  || v.HasMember("default color"))
  {
    Value& oa = v.HasMember("default color") ? v["default color"] : v["color"];
    default_color.x = oa[0].GetDouble();
    default_color.y = oa[1].GetDouble();
    default_color.z = oa[2].GetDouble();
    if (oa.Size() > 3)
      default_color.w = oa[3].GetDouble();
    else
      default_color.w = 1.0;
  }
  else
  {
    default_color.x = 0.8;
    default_color.y = 0.8;
    default_color.z = 0.8;
    default_color.w = 1.0;
  }

  if (v.HasMember("filename"))
  {
    return Import(v["filename"].GetString());
  }
  else
  {
    cerr << "ERROR: json Particles block has neither a filename nor a layout spec" << endl;
    exit(1);
  }

  return true;
}




} // namespace gxy
