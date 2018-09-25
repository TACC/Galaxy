// ========================================================================== //
// Copyright (c) 2014-2018 The University of Texas at Austin.                 //
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
#include <math.h>
#include <stdlib.h>
#include <string>
#include <sstream>
#include <fstream>
#include <sys/stat.h>
#include <fcntl.h>

#include "Application.h"
#include "Particles.h"

#include <vtkNew.h>
#include <vtkSmartPointer.h>
#include <vtkDataSetReader.h>
#include <vtkFloatArray.h>
#include <vtkIntArray.h>
#include <vtkPointData.h>
#include <vtkCharArray.h>
#include <vtkClientSocket.h>
#include <vtkXMLPolyDataReader.h>

#include "rapidjson/filereadstream.h"

using namespace std;
using namespace rapidjson;

namespace gxy
{

WORK_CLASS_TYPE(Particles::LoadPartitioningMsg);

KEYED_OBJECT_TYPE(Particles)

void
Particles::Register()
{
  RegisterClass();

	LoadPartitioningMsg::Register();
}

void
Particles::initialize()
{
  radius = 1;
	vtkobj = NULL;
  super::initialize();
}

void
Particles::allocate(int n)
{
	samples.resize(n);
}

Particles::~Particles()
{
	if (vtkobj) vtkobj->Delete();
}

int
Particles::serialSize()
{
	return super::serialSize() + sizeof(float);
}

unsigned char* 
Particles::serialize(unsigned char *ptr)
{
	ptr = super::serialize(ptr);
	*(float *)ptr = radius;
	ptr += sizeof(float);
	return ptr;
}

unsigned char* 
Particles::deserialize(unsigned char *ptr)
{
	ptr = super::deserialize(ptr);
	radius = *(float *)ptr;
	ptr += sizeof(float);
	return ptr;
}

void
Particles::LoadPartitioning(string p)
{
	LoadPartitioningMsg msg(getkey(), p);
  msg.Broadcast(true, true);
}

void
Particles::Import(string s)
{
	filename = s;

	struct stat info;
	if (stat(s.c_str(), &info))
	{
		cerr << "ERROR: Cannot open file " << s << endl;
		exit(1);
	} 

	int fd = open(s.c_str(), O_RDONLY);

	char *buf = new char[info.st_size + 1];
		
	char *p = buf;
	for (int m, n = info.st_size; n > 0; n -= m, p += m) m = read(fd, p, n);
	*p = 0;

	close(fd);

	set_attached(false);

	struct foo
	{
		float r[2];
		char s[2];
	};

	int sz = strlen(s.c_str()) + 1 + 2*sizeof(float);
	char *tmp = new char[sz];
	struct foo *args = (struct foo *)tmp;
	
	args->r[0] = radius;
	strcpy(args->s, s.c_str());

	Geometry::Import(buf, (void *)tmp, sz);

	delete[] tmp;
}

void
Particles::LoadFromJSON(Value& v)
{
  if (v.HasMember("radius"))
	{
    radius = v["radius"].GetDouble();
	}

  if (v.HasMember("filename"))
  {
		Import(v["filename"].GetString());
  }
  else if (v.HasMember("attach"))
  {
		Value& attach = v["attach"];
		if (!attach.HasMember("partitions") || !attach.HasMember("layout"))
		{
			cerr << "ERROR: Need both partitions and layout in attach clause" << endl;
			exit(1);
		}
    set_attached(true);
    LoadPartitioning(string(attach["partitions"].GetString()));
    Attach(string(attach["layout"].GetString()));
  }
  else
  {
    cerr << "ERROR: json Particles block has neither a filename nor a layout spec" << endl;
    exit(1);
  }
}

void
Particles::SaveToJSON(Value& v, Document& doc)
{
	Value container(kObjectType);

  if (attached)
    container.AddMember("layout", Value().SetString(filename.c_str(), doc.GetAllocator()), doc.GetAllocator());
  else
    container.AddMember("filename", Value().SetString(filename.c_str(), doc.GetAllocator()), doc.GetAllocator());

	container.AddMember("type", Value().SetString("Particles", doc.GetAllocator()), doc.GetAllocator());

	if (radius > 0)
		container.AddMember("radius", Value().SetDouble(radius), doc.GetAllocator());

	v.PushBack(container, doc.GetAllocator());
}

bool
Particles::local_commit(MPI_Comm c)
{
	if (Geometry::local_commit(c))
		return true; 
	OSPGeometry ospg = ospNewGeometry("spheres");

  int n_samples;
  Particle *samples;
  GetSamples(samples, n_samples);

  OSPData data = ospNewData(n_samples * sizeof(Particle), OSP_UCHAR, samples, OSP_DATA_SHARED_BUFFER);
  ospCommit(data);

  ospSetData(ospg, "spheres", data);

  ospSet1i(ospg, "offset_value", 12);
  ospSet1f(ospg, "radius", radius);

	srand(GetTheApplication()->GetRank());
	int r = random();

#if COLOR_BY_PARTITION
	unsigned int color = (r & 0x1 ? 0x000000ff : 0x000000A6) |
											 (r & 0x2 ? 0x0000ff00 : 0x0000A600) |
											 (r & 0x4 ? 0x00ff0000 : 0x00A60000) |
							         0xff000000;
#else
	unsigned int color = 0xffffffff;
#endif

	unsigned int *colors = new unsigned int[n_samples];
	for (int i = 0; i < n_samples; i++)
		colors[i] = color;

	OSPData clr = ospNewData(n_samples, OSP_UCHAR4, colors);
	delete[] colors;
	ospCommit(clr);
	ospSetObject(ospg, "color", clr);
	ospRelease(clr);

  ospCommit(ospg);

	theOSPRayObject = (OSPObject)ospg;

	return false;
}

bool
Particles::local_load_timestep(MPI_Comm c)
{
	if (vtkobj) vtkobj->Delete();
	vtkobj = NULL;

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

  vtkobj = vtkPolyData::New();
  vtkobj->ShallowCopy(vtkPolyData::SafeDownCast(reader->GetOutput()));
  if (! vtkobj)
  {
    cerr << "ERROR: load_local_timestep received something other than a vtkPolyData object" << endl;
    exit(1);
  }

  return false;
}

void
Particles::get_partitioning_from_file(char *s)
{
	Document *doc = new Document();

  FILE *pFile = fopen (s, "r");
  char buf[64];
  rapidjson::FileReadStream is(pFile, buf, 64);
  doc->ParseStream<0>(is);
  fclose(pFile);

	get_partitioning(*doc);

	delete doc;
}

void
Particles::get_partitioning(Value& doc)
{
  Value& parts = doc["parts"];
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
}

void
Particles::local_import(char *p, MPI_Comm c)
{
  string json(p);

  struct foo
  {
    float r[2];
    char s[2];
  };

  struct foo *args = (struct foo *)(p + strlen(p) + 1);

  radius = args->r[0];

  string f(args->s);
	string dir = (f.find_last_of("/") == f.npos) ? "./" : f.substr(0, f.find_last_of("/") + 1);

	if (vtkobj) vtkobj->Delete();
	vtkobj = NULL;

	samples.clear();

  Document doc;
  doc.Parse(json.c_str());

  get_partitioning(doc);

  const char *fname = doc["parts"][GetTheApplication()->GetRank()]["filename"].GetString();

	if (! strcmp(fname + (strlen(fname)-3), "sph"))
	{
		string f = dir + string(fname);

		struct stat info;
		if (stat(f.c_str(), &info))
		{
			cerr << "ERROR: Cannot open file " << f << endl;
			exit(1);
		}

		int n_samples = info.st_size / sizeof(Particle);
		samples.resize(n_samples);

		int fd = open(f.c_str(), O_RDONLY);

		char *p = (char *)samples.data();;
		int m, n = info.st_size;

		for (n = info.st_size; n > 0; n -= m, p += m)
		{
			m = read(fd, p, n);
			if (m < 0)
			{
				cerr << "Error reading file " << f.c_str() << endl;
				exit(1);
			}
		}

		close(fd);
	}
	else if (! strcmp(fname + (strlen(fname)-3), "vtp"))
	{
		string f = dir + string(fname);

		vtkXMLPolyDataReader *reader = vtkXMLPolyDataReader::New();
		reader->SetFileName(f.c_str());
		reader->Update();
		vtkobj = vtkPolyData::New();
		vtkobj->ShallowCopy(reader->GetOutput());
		reader->Delete();
	}
	else
	{
		cerr << "ERROR: Particles have to be in .sph or .vtp format" << endl;
		exit(1);
	}
}

void
Particles::GetPolyData(vtkPolyData*& v)
{
	if (! vtkobj)
	{
		Particle *ptr = get_samples();
		int n_samples = get_n_samples();

		vtkobj = vtkPolyData::New();
		vtkobj->Allocate(n_samples);

		vtkFloatArray *xyz = vtkFloatArray::New();
		xyz->SetNumberOfComponents(3);
		xyz->SetNumberOfTuples(n_samples);

		vtkFloatArray *val = vtkFloatArray::New();
		val->SetName("value");
		val->SetNumberOfComponents(1);
		val->SetNumberOfTuples(n_samples);

		float *pxyz = (float *)xyz->GetVoidPointer(0);
		float *pval = (float *)val->GetVoidPointer(0);

		vtkIdList *ids = vtkIdList::New();
		ids->SetNumberOfIds(1);

		for (int i = 0; i < n_samples; i++, ptr++)
		{
			*pxyz++ = ptr->xyz.x;
			*pxyz++ = ptr->xyz.y;
			*pxyz++ = ptr->xyz.z;
			*pval++ = ptr->u.value;
			ids->SetId(0, i);
			vtkobj->InsertNextCell(VTK_VERTEX, ids);
		}

		vtkPoints *vtp = vtkPoints::New();
		vtp->SetData(xyz);
		xyz->Delete();

		vtkobj->SetPoints(vtp);
		vtp->Delete();

		vtkobj->GetPointData()->AddArray(val);
	}

	v = vtkobj;
}

void
Particles::GetSamples(Particle*& s, int& n)
{
	if (samples.empty() && (!vtkobj || vtkobj->GetNumberOfPoints() == 0))
		s = NULL, n = 0;
	else if (! samples.empty())
		s = samples.data(), n = samples.size();
	else
	{
		vtkPoints *pts = vtkobj->GetPoints();
		vtkFloatArray *parr = vtkFloatArray::SafeDownCast(pts->GetData());

		int indx;
		vtkFloatArray *varr = vtkFloatArray::SafeDownCast(vtkobj->GetPointData()->GetAbstractArray("value", indx));

		int n_samples = parr->GetNumberOfTuples();
		samples.resize(n_samples);

		Particle *dst = samples.data();
		float *pptr = (float *)parr->GetVoidPointer(0);
		float *vptr = (float *) (varr ? varr->GetVoidPointer(0) : NULL);

		for (int i = 0; i < n_samples; i++, dst++)
		{
			dst->xyz.x = *pptr++;
			dst->xyz.y = *pptr++;
			dst->xyz.z = *pptr++;
			dst->u.value = vptr ? *vptr++ : 0;
		}

		s = samples.data(); 
		n = samples.size();
	}
}

} // namespace gxy
