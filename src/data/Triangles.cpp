#include <iostream>
#include "vtkerror.h"
#include "Triangles.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>

#include <string>
#include <string.h>

#include <vtkCell.h>
#include <vtkFloatArray.h>
#include <vtkPointData.h>
#include <vtkPointSet.h>
#include <vtkPolyData.h>
#include <vtkXMLPolyDataReader.h>
#include <vtkUnstructuredGrid.h>
#include <vtkXMLUnstructuredGridReader.h>

using namespace std;
using namespace boost::property_tree;

namespace pvol
{

KEYED_OBJECT_TYPE(Triangles) 

void     
Triangles::Register()
{        
  RegisterClass();
}



void
Triangles::initialize()
{
	super::initialize();
	vertices = NULL;
	normals = NULL;
	triangles = NULL;
};

void
Triangles::local_import(char *f, MPI_Comm c)
{
	string filename(f);

	size_t i = filename.find_last_of('/');
	string spath(i == string::npos ? "." : filename.substr(0, i));

  i = filename.find_last_of('.');
  string ext(filename.substr(i+1));

  ptree tree;

  ifstream ifs(filename);
  read_xml(ifs, tree);
  ifs.close();

  string part = "";

	int k = -1;
	if (ext == "pvtu" || ext == "pvtp")
	{
		const char *section = (ext == "pvtu") ? "VTKFile.PUnstructuredGrid" : "VTKFile.PPolyData";
	
		BOOST_FOREACH(ptree::value_type const& it, tree.get_child(section))
		{
			if (it.first == "Piece")
			{
				k++;
				if (k == GetTheApplication()->GetRank())
				{
					part = it.second.get("<xmlattr>.Source", "none");
					break;
				}
			}
		}
	}
	else if (ext == "pvd")
	{
		BOOST_FOREACH(ptree::value_type const& it, tree.get_child("VTKFile.Collection"))
		{
			if (it.first == "DataSet")
			{
				k++;
				if (k == GetTheApplication()->GetRank())
				{
					part = it.second.get("<xmlattr>.file", "none");
					break;
				}
			}
		}
	}
	else 
	{
		cerr << "unknown extension: " << ext << "\n";
		exit(1);
	}

	if (k != GetTheApplication()->GetRank())
	{
		cerr << "Not enough partitions\n";
		exit(1);
	}

	vtkPointSet *pset = NULL;

  i = part.find_last_of('.');
  ext = part.substr(i+1);

	if (ext == "vtu")
	{
		VTKError *ve = VTKError::New();
		vtkXMLUnstructuredGridReader* rdr = vtkXMLUnstructuredGridReader::New();
		ve->watch(rdr);

		rdr->SetFileName((spath + "/" + part).c_str());
		rdr->Update();

		if (ve->GetError())
		{
			cerr << "error reading partition\n";
			exit(1);
		}

		vtkUnstructuredGrid *ug = rdr->GetOutput();
		ug->RemoveGhostCells();

		pset = (vtkPointSet *)ug;
		pset->Register(pset);
	}
	else if (ext == "vtp")
	{
		VTKError *ve = VTKError::New();

		vtkXMLPolyDataReader* rdr = vtkXMLPolyDataReader::New();
		ve->watch(rdr);

		rdr->SetFileName((spath + "/" + part).c_str());
		rdr->Update();

		if (ve->GetError())
		{
			cerr << "error reading partition\n";
			exit(1);
		}

		vtkPolyData *pd = rdr->GetOutput();
		pd->RemoveGhostCells();

		pset = (vtkPointSet *)pd;
		pset->Register(pset);
	}
	else 
	{
		cerr << "unknown partition extension: " << ext << "\n";
		exit(1);
	}

	if (k != GetTheApplication()->GetRank())
	{
		cerr << "Not enough partitions\n";
		exit(1);
	}

  vtkPoints* pts = pset->GetPoints();
  vtkFloatArray *parray  = vtkFloatArray::SafeDownCast(pset->GetPoints()->GetData());

  vtkFloatArray *narray = vtkFloatArray::SafeDownCast(pset->GetPointData()->GetArray("Normals"));

	int n_original_vertices = pset->GetNumberOfPoints();
	n_triangles = pset->GetNumberOfCells();

	int *vmap = new int[n_original_vertices];
	for (int i = 0; i < n_original_vertices; i++)
		vmap[i] = -1;

	triangles = new int[3*n_triangles];
	n_vertices = 0;

  for (int i = 0; i < n_triangles; i++)
  {
    if (pset->GetCellType(i) != VTK_TRIANGLE)
     cerr << i << "IS BAD\n";
    else
		{
      vtkCell *c = pset->GetCell(i);
      for (int j = 0; j < 3; j++)
			{
				int t = c->GetPointId(j);
				if (vmap[t] == -1)
					vmap[t] = n_vertices++;

				t = vmap[t];
				triangles[i*3+j] = t;
			}
    }
  }

  float *orig_vertices = (float *)parray->GetVoidPointer(0);
  float *orig_normals  = (float *)narray->GetVoidPointer(0);

	vertices = new float[3*n_vertices];
	normals = new float[3*n_vertices];

	bool first = true;
	float *po = orig_vertices;
	float *no = orig_normals;
	for (int i = 0; i < n_original_vertices; i++)
	{
		if (vmap[i] != -1)
		{
			if (first)
			{
				first = false;
				local_box.xyz_min.x = local_box.xyz_max.x = po[0];
				local_box.xyz_min.y = local_box.xyz_max.y = po[1];
				local_box.xyz_min.z = local_box.xyz_max.z = po[2];
			}
			else
			{
				if (po[0] < local_box.xyz_min.x) local_box.xyz_min.x = po[0];
				if (po[0] > local_box.xyz_max.x) local_box.xyz_max.x = po[0];
				if (po[1] < local_box.xyz_min.y) local_box.xyz_min.y = po[1];
				if (po[1] > local_box.xyz_max.y) local_box.xyz_max.y = po[1];
				if (po[2] < local_box.xyz_min.z) local_box.xyz_min.z = po[2];
				if (po[2] > local_box.xyz_max.z) local_box.xyz_max.z = po[2];
			}

			float *pn = vertices + 3*vmap[i];
			*pn++ = *po++;
			*pn++ = *po++;
			*pn++ = *po++;

			float *nn = normals + 3*vmap[i];
			*nn++ = *no++;
			*nn++ = *no++;
			*nn++ = *no++;
		}
		else
		{
			po += 3;
			no += 3;
		}
	}

	delete[] vmap;


	float box[] = {
		local_box.xyz_min.x, local_box.xyz_min.y, local_box.xyz_min.z,
		local_box.xyz_max.x, local_box.xyz_max.y, local_box.xyz_max.z
	};

	float *distributed_boxes = new float[GetTheApplication()->GetSize() * sizeof(box)];

	MPI_Allgather((const void *)&box, sizeof(box), MPI_CHAR, (void *)distributed_boxes, sizeof(box), MPI_CHAR, c);

	for (int i = 0; i < 6; i++) neighbors[i] = -1;

#define FUZZ 0.001
#define LAST(AXIS) (fabs(b.xyz_max.AXIS - local_box.xyz_min.AXIS) < FUZZ)
#define NEXT(AXIS) (fabs(b.xyz_min.AXIS - local_box.xyz_max.AXIS) < FUZZ)
#define EQ(AXIS)   (fabs(b.xyz_min.AXIS - local_box.xyz_min.AXIS) < FUZZ)

	global_box = local_box;

	for (int i = 0; i < GetTheApplication()->GetSize(); i++)
	{
		if (i != GetTheApplication()->GetRank())
		{
			Box b(distributed_boxes + i*6);

			if (global_box.xyz_min.x > b.xyz_min.x) global_box.xyz_min.x = b.xyz_min.x;
			if (global_box.xyz_max.x < b.xyz_max.x) global_box.xyz_max.x = b.xyz_max.x;
			if (global_box.xyz_min.y > b.xyz_min.y) global_box.xyz_min.y = b.xyz_min.y;
			if (global_box.xyz_max.y < b.xyz_max.y) global_box.xyz_max.y = b.xyz_max.y;
			if (global_box.xyz_min.z > b.xyz_min.z) global_box.xyz_min.z = b.xyz_min.z;
			if (global_box.xyz_max.z < b.xyz_max.z) global_box.xyz_max.z = b.xyz_max.z;


			if (LAST(x) && EQ(y) && EQ(z)) neighbors[0] = i;
			if (NEXT(x) && EQ(y) && EQ(z)) neighbors[1] = i;

			if (EQ(x) && LAST(y) && EQ(z)) neighbors[2] = i;
			if (EQ(x) && NEXT(y) && EQ(z)) neighbors[3] = i;

			if (EQ(x) && EQ(y) && LAST(z)) neighbors[4] = i;
			if (EQ(x) && EQ(y) && NEXT(z)) neighbors[5] = i;
			
		}
	}
}

bool
Triangles::local_commit(MPI_Comm c)
{
  if (Geometry::local_commit(c))
    return true;

  float *cptr, *colors = new float[n_vertices * 4];
  cptr = colors;
  for (int i = 0; i < n_vertices; i++)
  {
    *cptr++ = 1.0;
    *cptr++ = 1.0;
    *cptr++ = 1.0;
    *cptr++ = 1.0;
  } 
     
  OSPData pdata = ospNewData(n_vertices, OSP_FLOAT3, vertices, OSP_DATA_SHARED_BUFFER);
  ospCommit(pdata);
      
  OSPData ndata = ospNewData(n_vertices, OSP_FLOAT3, normals, OSP_DATA_SHARED_BUFFER); 
  ospCommit(ndata);
  
  OSPData tdata = ospNewData(n_triangles, OSP_INT3, triangles);
  ospCommit(tdata);
  
  OSPData cdata = ospNewData(n_vertices, OSP_FLOAT4, colors);
  ospCommit(cdata);
  delete[] colors;
    
  OSPGeometry ospg = ospNewGeometry("triangles");

  ospSetData(ospg, "vertex", pdata);
  ospSetData(ospg, "vertex.normal", ndata);
  ospSetData(ospg, "index", tdata);
  ospSetData(ospg, "color", cdata);

	ospCommit(ospg);

  theOSPRayObject = (OSPObject)ospg;
	return false;
} 

void
Triangles::LoadFromJSON(Value& v)
{ 
  if (v.HasMember("filename"))
  { 
    Import(v["filename"].GetString());
  }
  else if (v.HasMember("attach"))
  { 
		std::cerr << "attaching triangles source is not implemented\n";
    exit(1);
  }
  else
  { 
    std::cerr << "json triangles has neither filename or layout spec\n";
  }
}

void
Triangles::SaveToJSON(Value& v, Document& doc)
{
  Value container(kObjectType);

	container.AddMember("filename", Value().SetString(filename.c_str(), doc.GetAllocator()), doc.GetAllocator());
	v.PushBack(container, doc.GetAllocator());
}

Triangles::~Triangles()
{
	if (triangles) delete[] triangles;
	if (vertices) delete[] vertices;
	if (normals) delete[] normals;
}

}
