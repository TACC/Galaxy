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

using namespace boost::property_tree;
using namespace rapidjson;
using namespace std;

namespace gxy
{

OBJECT_CLASS_TYPE(Triangles) 

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
  float *dboxes = new float[6 * GetTheApplication()->GetSize()];

  size_t i = filename.find_last_of('/') + 1;
  string spath(i == string::npos ? "./" : filename.substr(0, i));

  i = filename.find_last_of('.');
  string ext(filename.substr(i+1));

  string part = "";

  int k = -1;
  if (ext == "pvtu" || ext == "pvtp" || ext == "pvd")
  {
    ptree tree;

    ifstream ifs(filename);
    read_xml(ifs, tree);
    ifs.close();

    const char *section = (ext == "pvtu") ? "VTKFile.PUnstructuredGrid" : (ext == "pvtp") ? "VTKFile.PPolyData" : "VTKFile.Collection";
    const char *tag     = (ext == "pvtu" || ext == "pvtp") ? "Piece" : "DataSet";
    const char *attr    = (ext == "pvtu" || ext == "pvtp") ? "<xmlattr>.Source" : "<xmlattr>.file";

    BOOST_FOREACH(ptree::value_type const& it, tree.get_child(section))
    {
      if (it.first == tag)
      {
        k++;
        if (k == GetTheApplication()->GetRank())
        {
          part = it.second.get(attr, "none");
          break;
        }
      }
    }

    if (k != GetTheApplication()->GetRank())
    {
      cerr << "ERROR: Not enough partitions for Triangles" << endl;
      exit(1);
    }

  }
  else if (ext == "ptri")
  {
    ifstream ifs;
    ifs.open(filename);

    string s;
    while (getline(ifs, s))
    {
      k++;
      stringstream ss(s);
      ss >> dboxes[6*k + 0] >> dboxes[6*k + 1] 
         >> dboxes[6*k + 2] >> dboxes[6*k + 3] 
         >> dboxes[6*k + 4] >> dboxes[6*k + 5] 
         >> s;
      if (k == GetTheApplication()->GetRank())
        part = s;
    }

    ifs.close();

    if (k != (GetTheApplication()->GetSize() - 1))
    {
      cerr << "ERROR: wrong number of Triangle partitions" << k << endl;
      exit(1);
    }
  }
  else 
  {
    cerr << "ERROR: unknown Triangle extension: " << ext << endl;
    exit(1);
  }

  vtkPointSet *pset = NULL;

	part = spath + part;

  i = part.find_last_of('.');
  string pext = part.substr(i+1);

  if (pext == "vtu")
  {
    VTKError *ve = VTKError::New();
    vtkXMLUnstructuredGridReader* rdr = vtkXMLUnstructuredGridReader::New();
    ve->watch(rdr);

    rdr->SetFileName(part.c_str());
    rdr->Update();

    if (ve->GetError())
    {
      cerr << "ERROR: error reading vtu Triangle partition" << endl;
      exit(1);
    }

    vtkUnstructuredGrid *ug = rdr->GetOutput();
    ug->RemoveGhostCells();

    pset = (vtkPointSet *)ug;
    pset->Register(pset);
  }
  else if (pext == "vtp")
  {
    VTKError *ve = VTKError::New();

    vtkXMLPolyDataReader* rdr = vtkXMLPolyDataReader::New();
    ve->watch(rdr);

    rdr->SetFileName(part.c_str());
    rdr->Update();

    if (ve->GetError())
    {
      cerr << "ERROR: error reading vtp Triangle partition" << endl;
      exit(1);
    }

    vtkPolyData *pd = rdr->GetOutput();
    pd->RemoveGhostCells();

    pset = (vtkPointSet *)pd;
    pset->Register(pset);
  }
  else 
  {
    cerr << "ERROR: unknown Triangle partition extension: " << pext << endl;
    exit(1);
  }

	if (pset->GetNumberOfPoints() == 0)
		n_vertices = 0;
	else
	{
		vtkPoints* pts = pset->GetPoints();

		vtkFloatArray *parray = vtkFloatArray::SafeDownCast(pset->GetPoints()->GetData());
		vtkFloatArray *narray = vtkFloatArray::SafeDownCast(pset->GetPointData()->GetArray("Normals"));
		if (! narray)
			narray = vtkFloatArray::SafeDownCast(pset->GetPointData()->GetArray("Normals_"));

		if (!narray)
		{
			cerr << "ERROR: Triangle partition has no normals" << endl;
			exit(1);
		}

		int n_original_vertices = pset->GetNumberOfPoints();
		n_triangles = pset->GetNumberOfCells();

		int *vmap = new int[n_original_vertices];
		for (int i = 0; i < n_original_vertices; i++)
			vmap[i] = -1;

		// NOTE: we're going to make sure here that there aren't any unreferenced vertices
		
		triangles = new int[3*n_triangles];
		n_vertices = 0;

		for (int i = 0; i < n_triangles; i++)
		{
			if (pset->GetCellType(i) != VTK_TRIANGLE)
			 cerr << "WARNING: cell " << i << "is not type VTK_TRIANGLE" << endl;
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
	}

		// Now the opartition is loaded.   If this ISN'T a ptri file containing
		// BB's, we compute the BB based on the vertices.
		
	if (ext == "ptri")
  {
    local_box = Box(dboxes + 6*GetTheApplication()->GetRank());
  }
  else if (n_vertices == 0)
	{
		cerr << "ERROR: Can't figure out bounding box" << endl;
		exit(1);
	}
	else
  {
    bool first = true;
    float *v = vertices;
    for (int i = 0; i < n_vertices; i++)
      if (first)
      {
        first = false;
        local_box.xyz_min.x = local_box.xyz_max.x = v[0];
        local_box.xyz_min.y = local_box.xyz_max.y = v[1];
        local_box.xyz_min.z = local_box.xyz_max.z = v[2];
      }
      else
      {
        if (v[0] < local_box.xyz_min.x) local_box.xyz_min.x = v[0];
        if (v[0] > local_box.xyz_max.x) local_box.xyz_max.x = v[0];
        if (v[1] < local_box.xyz_min.y) local_box.xyz_min.y = v[1];
        if (v[1] > local_box.xyz_max.y) local_box.xyz_max.y = v[1];
        if (v[2] < local_box.xyz_min.z) local_box.xyz_min.z = v[2];
        if (v[2] > local_box.xyz_max.z) local_box.xyz_max.z = v[2];
      }

		float box[] = {
			local_box.xyz_min.x, local_box.xyz_min.y, local_box.xyz_min.z,
			local_box.xyz_max.x, local_box.xyz_max.y, local_box.xyz_max.z
			};

			MPI_Allgather((const void *)&box, sizeof(box), MPI_CHAR, (void *)dboxes, sizeof(box), MPI_CHAR, c);
  }

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
      Box b(dboxes + i*6);

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

void
Triangles::LoadFromJSON(Value& v)
{ 
  if (v.HasMember("filename"))
  { 
    Import(v["filename"].GetString());
  }
  else if (v.HasMember("attach"))
  { 
    cerr << "ERROR: attaching triangles source is not implemented" << endl;
    exit(1);
  }
  else
  { 
    cerr << "ERROR: json triangles has neither filename or layout spec" << endl;
    exit(1);
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

} // namespace gxy
