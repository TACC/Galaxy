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
#include <vtkerror.h>
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

KEYED_OBJECT_CLASS_TYPE(Particles)

void
Particles::Register()
{
  RegisterClass();

	LoadPartitioningMsg::Register();
}

void
Particles::initialize()
{
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
	return ptr;
}

unsigned char* 
Particles::deserialize(unsigned char *ptr)
{
	ptr = super::deserialize(ptr);
	return ptr;
}

bool
Particles::LoadPartitioning(string p)
{
	LoadPartitioningMsg msg(getkey(), p);
  msg.Broadcast(true, true);
  return get_error() == 0;
}

bool
Particles::Import(string s)
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

	bool r = Geometry::Import(buf, (void *)s.c_str(), s.size()+1);

  return r;
}

bool
Particles::LoadFromJSON(Value& v)
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
  else if (v.HasMember("attach"))
  {
		Value& attach = v["attach"];
		if (!attach.HasMember("partitions") || !attach.HasMember("layout"))
		{
			cerr << "ERROR: Need both partitions and layout in attach clause" << endl;
			exit(1);
		}
    set_attached(true);
    if (! LoadPartitioning(string(attach["partitions"].GetString())))
      return false;
    
    if (! Attach(string(attach["layout"].GetString())))
      return false;
  }
  else
  {
    cerr << "ERROR: json Particles block has neither a filename nor a layout spec" << endl;
    exit(1);
  }

  return true;
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

  Value c(kArrayType);
  c.PushBack(Value().SetDouble(default_color.x), doc.GetAllocator());
  c.PushBack(Value().SetDouble(default_color.y), doc.GetAllocator());
  c.PushBack(Value().SetDouble(default_color.z), doc.GetAllocator());
  c.PushBack(Value().SetDouble(default_color.w), doc.GetAllocator());
  container.AddMember("default color", c, doc.GetAllocator());


	v.PushBack(container, doc.GetAllocator());
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
    return false;

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
    set_error(1);
    return false;
  }

  return true;
}

bool
Particles::local_import(char *p, MPI_Comm c)
{
  int rank = GetTheApplication()->GetRank();

  string json(p);
  string filename(p + strlen(p) + 1);
	string dir = (filename.find_last_of("/") == filename.npos) ? "./" : filename.substr(0, filename.find_last_of("/") + 1);

	if (vtkobj) vtkobj->Delete();
	vtkobj = NULL;

	samples.clear();

  Document doc;
  if (doc.Parse<0>(json.c_str()).HasParseError() || !get_partitioning(doc))
  {
    if (rank == 0) std::cerr << "parse error in partition document: " << json << "\n";
    set_error(1);
    return false;
  }

  // Checks in get_partitioning ensures this is OK
  const char *fname = doc["parts"][GetTheApplication()->GetRank()]["filename"].GetString();

	if (! strcmp(fname + (strlen(fname)-3), "sph"))
	{
		string f = dir + string(fname);

		struct stat info;
		if (stat(f.c_str(), &info))
		{
			cerr << "ERROR " << rank << ": Cannot open file " << f << endl;
			return false;
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
				cerr << "ERROR " << rank << ": Error reading file " << f.c_str() << endl;
				return false;
			}
		}

		close(fd);
	}
	else if (! strcmp(fname + (strlen(fname)-3), "vtp"))
	{
		string f = dir + string(fname);

    VTKError *ve = VTKError::New();
		vtkXMLPolyDataReader *reader = vtkXMLPolyDataReader::New();
    ve->watch(reader);

    reader->SetFileName(f.c_str());
    reader->Update();

    if (ve->GetError())
    {
      cerr << "ERROR " << rank << ": error reading vtu Triangle partition" << endl;
      return false;
    }

		vtkobj = vtkPolyData::New();
		vtkobj->ShallowCopy(reader->GetOutput());
		reader->Delete();
	}
	else
	{
		cerr << "ERROR " << rank << ": Particles have to be in .sph or .vtp format" << endl;
		return false;
	}

  return true;
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

bool
Particles::local_commit(MPI_Comm c)
{
  int n;
  Particle *d;

  GetSamples(d, n);

  float lmin;
  float lmax;

  if (n == 0)
  {
    lmin = 0;
    lmax = 0;
  }
  else
  {
    lmin = d->u.value;
    lmax = d->u.value;
    d++;
    for (int i = 1; i < n; i++, d++)
    {
      float v = d->u.value;
      if (v > lmax) lmax = v;
      if (v < lmin) lmin = v;
    }
  }
    
  float gmin, gmax;
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

  set_local_minmax(gmin, gmax);
  set_global_minmax(gmin, gmax);
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
