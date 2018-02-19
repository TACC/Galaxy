#include <iostream>
#include <math.h>
#include <stdlib.h>
#include <string>
#include <sstream>
#include <fstream>
#include <sys/stat.h>
#include <fcntl.h>

#include "Application.h"
// #include "Renderer.h"
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

#include "../rapidjson/filestream.h"

WORK_CLASS_TYPE(Particles::LoadPartitioningMsg);

KEYED_OBJECT_TYPE(Particles)

using namespace std;

void
Particles::Register()
{
  RegisterClass();

	LoadPartitioningMsg::Register();
}

void
Particles::initialize()
{
  samples = NULL;
  n_samples = 0;
  radius_scale = 1;
  radius = 0;
	vtkobj = NULL;
  super::initialize();
}

void
Particles::allocate(int n)
{
	n_samples = n;

	if (samples)
		delete[] samples;

	samples  = new Particle[n_samples];
}

Particles::~Particles()
{
	if (vtkobj) vtkobj->Delete();
  if (samples) free(samples);
}

void
Particles::LoadPartitioning(string p)
{
	LoadPartitioningMsg msg(getkey(), p);
  msg.Broadcast(true);
}

void
Particles::Import(string s)
{
	filename = s;

	struct stat info;
	if (stat(s.c_str(), &info))
	{
		cerr << "Cannot open " << s << "\n";
		exit(1);
	} 

	int fd = open(s.c_str(), O_RDONLY);

	char *buf = new char[info.st_size + 1];
		
	char *p = buf;
	for (int m, n = info.st_size; n > 0; n -= m, p += m) m = read(fd, p, n);
	*p = 0;

	close(fd);

	set_attached(false);

	float r[2];
	r[0] = radius_scale;
	r[1] = radius;
	Geometry::Import(buf, (void *)r, sizeof(r));

	delete[] buf;
}

void
Particles::LoadFromJSON(Value& v)
{
  if (v.HasMember("radius_scale"))
    radius_scale = v["radius_scale"].GetDouble();

  if (v.HasMember("radius"))
    radius = v["radius"].GetDouble();

  if (v.HasMember("filename"))
  {
		Import(v["filename"].GetString());
  }
  else if (v.HasMember("attach"))
  {
		Value& attach = v["attach"];
		if (!attach.HasMember("partitions") || !attach.HasMember("layout"))
		{
			std::cerr << "Need both partitions and layout in attach clause\n";
			exit(0);
		}
    set_attached(true);
    LoadPartitioning(string(attach["partitions"].GetString()));
    Attach(string(attach["layout"].GetString()));
  }
  else
  {
    std::cerr << "json Particles has neither filename or layout spec\n";
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

	if (radius_scale > 0)
		container.AddMember("radius_scale", Value().SetDouble(radius_scale), doc.GetAllocator());

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
  ospSet1f(ospg, "radius_scale", radius_scale);
  ospSet1f(ospg, "radius", radius);
	std::cerr << "radius: " << radius << "\n";

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
    cerr << "received something other than a vtkPolyData object\n";
    exit(1);
  }

  return false;
}

void
Particles::get_partitioning_from_file(char *s)
{
	Document *doc = new Document();

  FILE *pFile = fopen (s, "r");
  rapidjson::FileStream is(pFile);
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
  void *args = p + strlen(p) + 1;

  radius_scale = ((float *)args)[0];
  radius = ((float *)args)[1];

	std::cerr << radius <<  " " << radius_scale << "\n";

	if (vtkobj) vtkobj->Delete();
	vtkobj = NULL;

	if (samples) delete[] samples;
	samples = NULL;

  Document doc;
  doc.Parse(json.c_str());

  if (samples) delete samples;

  get_partitioning(doc);

  const char *fname = doc["parts"][GetTheApplication()->GetRank()]["filename"].GetString();

	if (! strcmp(fname + (strlen(fname)-3), "sph"))
	{
		struct stat info;
		if (stat(fname, &info))
		{
			cerr << "Cannot open " << fname << "\n";
			exit(1);
		}

		n_samples = info.st_size / sizeof(Particle);
		samples  = new Particle[n_samples];

		int fd = open(fname, O_RDONLY);

		char *p = (char *)samples;
		int m, n = info.st_size;

		for (n = info.st_size; n > 0; n -= m, p += m)
		{
			m = read(fd, p, n);
			if (m < 0)
			{
				cerr << "Error reading " << fname << "\n";
				exit(1);
			}
		}

		close(fd);
	}
	else if (! strcmp(fname + (strlen(fname)-3), "vtp"))
	{
		vtkXMLPolyDataReader *reader = vtkXMLPolyDataReader::New();
		reader->SetFileName(fname);
		reader->Update();
		vtkobj = vtkPolyData::New();
		vtkobj->ShallowCopy(reader->GetOutput());
		reader->Delete();
	}
	else
	{
		std::cerr << "Particles have to be .sph or .vtp\n";
		exit(0);
	}
}

void
Particles::GetPolyData(vtkPolyData*& v)
{
	if (! vtkobj)
	{
		if (! samples)
		{
			std::cerr << "got to have samples or vtk\n";
			exit(0);
		}

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

		for (int i = 0; i < n_samples; i++)
		{
			*pxyz++ = samples[i].xyz.x;
			*pxyz++ = samples[i].xyz.y;
			*pxyz++ = samples[i].xyz.z;
			*pval++ = samples[i].u.value;
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
	if (! samples)
	{
		if (! vtkobj)
		{
			std::cerr << "got to have samples or vtk\n";
			exit(0);
		}

		vtkPoints *pts = vtkobj->GetPoints();
		vtkFloatArray *parr = vtkFloatArray::SafeDownCast(pts->GetData());

		int indx;
		vtkFloatArray *varr = vtkFloatArray::SafeDownCast(vtkobj->GetPointData()->GetAbstractArray("value", indx));

		n_samples = parr->GetNumberOfTuples();
		samples = new Particle[n_samples];

		Particle *dst = samples;
		float *pptr = (float *)parr->GetVoidPointer(0);
		float *vptr = (float *) (varr ? varr->GetVoidPointer(0) : NULL);

		for (int i = 0; i < n_samples; i++, dst++)
		{
			dst->xyz.x = *pptr++;
			dst->xyz.y = *pptr++;
			dst->xyz.z = *pptr++;
			dst->u.value = vptr ? *vptr++ : 0;
		}
	}

	s = samples; 
	n = n_samples;
}

