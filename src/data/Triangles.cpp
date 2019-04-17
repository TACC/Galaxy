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

KEYED_OBJECT_CLASS_TYPE(Triangles) 

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

bool
Triangles::local_import(char *p, MPI_Comm c)
{
  int rank = GetTheApplication()->GetRank();

  string json(p);
  string filename(p + strlen(p) + 1);
  string dir = (filename.find_last_of("/") == filename.npos) ? "./" : filename.substr(0, filename.find_last_of("/") + 1);

  if (vtkobj) vtkobj->Delete();
  vtkobj = NULL;

  if (triangles) { delete[] triangles; triangles = nullptr; }
  if (normals) { delete[] normals; normals = nullptr; }
  if (vertices) { delete[] vertices; vertices = nullptr; }

  Document doc;
  if (doc.Parse<0>(json.c_str()).HasParseError() || !get_partitioning(doc))
  {
    if (rank == 0) std::cerr << "parse error in partition document: " << json << "\n";
    set_error(1);
    return false;
  }

  // Checks in get_partitioning ensures this is OK
  string fname = doc["parts"][GetTheApplication()->GetRank()]["filename"].GetString();
  if (fname[0] != '/')
    fname = dir + fname;

  vtkPointSet *pset = NULL;

  string pext = fname.substr(fname.find_last_of('.') + 1);
  if (pext == "vtu")
  {
    VTKError *ve = VTKError::New();
    vtkXMLUnstructuredGridReader* rdr = vtkXMLUnstructuredGridReader::New();
    ve->watch(rdr);

    rdr->SetFileName(fname.c_str());
    rdr->Update();

    if (ve->GetError())
    {
      cerr << "ERROR " << rank << ": error reading vtu Triangle partition" << endl;
      return false;
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

    rdr->SetFileName(fname.c_str());
    rdr->Update();

    if (ve->GetError())
    {
      cerr << "ERROR " << rank << ": error reading vtu Triangle partition" << endl;
      return false;
    }

    vtkPolyData *pd = rdr->GetOutput();
    pd->RemoveGhostCells();

    pset = (vtkPointSet *)pd;
    pset->Register(pset);
  }
  else 
  {
    cerr << "ERROR " << rank << ": unknown Triangle partition extension: " << pext << endl;
    return false;
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
      cerr << "ERROR " << rank << ": Triangle partition has no normals: " << pext << endl;
      return false;
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
#if 0
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
#endif

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

  return true;
}

bool
Triangles::LoadFromJSON(Value& v)
{ 
  if (v.HasMember("filename"))
  { 
    return Import(v["filename"].GetString());
  }
  else if (v.HasMember("attach"))
  { 
    cerr << "ERROR: attaching triangles source is not implemented" << endl;
    set_error(1);
    return false;
  }
  else
  { 
    cerr << "ERROR: json triangles has neither filename or layout spec" << endl;
    set_error(1);
    return false;
  }
}

Triangles::~Triangles()
{
  if (triangles) delete[] triangles;
  if (vertices) delete[] vertices;
  if (normals) delete[] normals;
}

} // namespace gxy
