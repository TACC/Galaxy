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
#include "PathLines.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>

#include <string>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <vtkCell.h>
#include <vtkFloatArray.h>
#include <vtkDoubleArray.h>
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

KEYED_OBJECT_CLASS_TYPE(PathLines) 

void     
PathLines::Register()
{        
  RegisterClass();
}

void
PathLines::initialize()
{
  super::initialize();
  vertices_shared = false;
  segments_shared = false;

  vertices = NULL;
  segments = NULL;

  vtkug = NULL;
};

bool
PathLines::Import(string s)
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
PathLines::local_import(char *p, MPI_Comm c)
{
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
  string fname(doc["parts"][GetTheApplication()->GetRank()]["filename"].GetString());
  string ext(fname.substr(fname.size() - 3));
  std::cerr << "FILENAME: " << fname << " EXT: " << ext << "\n";

  if (fname.c_str()[0] != '/')
    fname = dir + fname;

  if (ext == "vtu")
  {
    VTKError *ve = VTKError::New();
    vtkXMLUnstructuredGridReader* rdr = vtkXMLUnstructuredGridReader::New();
    ve->watch(rdr);

    rdr->SetFileName(fname.c_str());
    rdr->Update();

    if (ve->GetError())
    {
      cerr << "ERROR " << rank << ": error reading vtu PathLine partition" << endl;
      return false;
    }

    vtkug = rdr->GetOutput();
    vtkug->RemoveGhostCells();
    vtkug->Register(vtkug);

    if (vtkug->GetNumberOfPoints() == 0)
    {
      n_vertices = 0;
      InstallVertexData(false, 0, NULL);
      InstallSegments(false, 0, NULL);
    }
    else
    {
      int nv = vtkug->GetNumberOfPoints();
      int npl = vtkug->GetNumberOfCells();

      // This sucks, but we need to rearrange the vertices so that they are
      // sequential along each pathline.   VTK avoids this by using a level 
      // of indirection, but OSPRay doesn't.   First, get the list of vertex
      // references...

      int nrefs = 0;
      for (int i = 0; i < npl; i++)
      {
        vtkCell *c = vtkug->GetCell(i);
        nrefs += c->GetNumberOfPoints();
      }

      // Now get the points and (if present) the point-dependent scalar data array

      vtkFloatArray *parray = vtkFloatArray::SafeDownCast(vtkug->GetPoints()->GetData());
      float *vertexData = new float[4*nrefs];

      vtkDataArray *array = vtkug->GetPointData()->GetScalars();
      if (!array) array = vtkug->GetPointData()->GetArray("Data");
      if (array && array->GetNumberOfComponents() > 1) array = NULL;

      vtkFloatArray *farray = vtkFloatArray::SafeDownCast(array);
      if (farray)
      {
         float *data = (float *)farray->GetVoidPointer(0);

         int refs[nrefs];
         for (int k = 0, i = 0; i < npl; i++)
         {
           vtkCell *c = vtkug->GetCell(i);
           for (int j = 0; j < c->GetNumberOfPoints(); j++, k++)
           {
             int id = c->GetPointId(j);
             parray->GetTypedTuple(id, vertexData + k*4);
             vertexData[k*4 + 3] = data[k];
           }
         }
      }
      else 
      {
        vtkDoubleArray *darray = vtkDoubleArray::SafeDownCast(array);
        if (darray)
        {
          double *data = (double *)darray->GetVoidPointer(0);

          int refs[nrefs];
          for (int k = 0, i = 0; i < npl; i++)
          {
            vtkCell *c = vtkug->GetCell(i);
            for (int j = 0; j < c->GetNumberOfPoints(); j++, k++)
            {
              int id = c->GetPointId(j);
              parray->GetTypedTuple(id, vertexData + k*4);
              vertexData[k*4 + 3] = (float)data[k];
            }
          }
        }
        else
        {
          int refs[nrefs];
          for (int k = 0, i = 0; i < npl; i++)
          {
            vtkCell *c = vtkug->GetCell(i);
            for (int j = 0; j < c->GetNumberOfPoints(); j++, k++)
            {
              int id = c->GetPointId(j);
              parray->GetTypedTuple(id, vertexData + k*4);
              vertexData[k*4 + 3] = 0;
            }
          }
        }
      }

      // nrefs is the total number of vertices in all the pathlines.   The number of *segments*
      // is that minus one for each pathline

      int nseg = nrefs - npl;
      int *segs = new int[nseg];

      for (int s = 0, v = 0, i = 0; i < npl; i++)
      {
        vtkCell *c = vtkug->GetCell(i);
        for (int j = 0; j < c->GetNumberOfPoints(); j++, v++)
          if (j != (c->GetNumberOfPoints()-1)) segs[s++] = v;
      }

      InstallVertexData(false, nrefs, vertexData);
      InstallSegments(false, nseg, segs);
    }
  }
  else 
  {
    std::cerr << "bad file extension for pathline data\n";
    exit(1);
  }

  return true;
}

bool
PathLines::LoadFromJSON(Value& v)
{ 
  if (v.HasMember("filename"))
  { 
    return Import(v["filename"].GetString());
  }
  else if (v.HasMember("attach"))
  { 
    cerr << "ERROR: attaching pathline source is not implemented" << endl;
    set_error(1);
    return false;
  }
  else
  { 
    cerr << "ERROR: json pathline has neither filename or layout spec" << endl;
    set_error(1);
    return false;
  }
}

void
PathLines::SaveToJSON(Value& v, Document& doc)
{
  Value container(kObjectType);

  container.AddMember("filename", Value().SetString(filename.c_str(), doc.GetAllocator()), doc.GetAllocator());
  v.PushBack(container, doc.GetAllocator());
}

PathLines::~PathLines()
{
  if (vertices_shared && vertices) delete[] vertices;
  if (segments_shared && segments) delete[] segments;
  if (vtkug) vtkug->Delete();
}

void
PathLines::InstallSegments(bool shared, int ns, int *s)
{
  if (segments_shared && segments) delete[] segments;

  segments = s;
  n_segments = ns;

  segments_shared = shared;
}

void
PathLines::InstallVertexData(bool shared, int nv, float *v)
{
  if (vertices_shared)
  {
    if (vertices) delete[] vertices;
  }

  vertices = v;

  n_vertices = nv;
  vertices_shared = shared;
}

} // namespace gxy
