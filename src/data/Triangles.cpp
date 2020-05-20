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
#include "OsprayTriangles.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>

#include <string>
#include <string.h>

#include <vtkCell.h>
#include <vtkCellType.h>
#include <vtkCellIterator.h>
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
};

Triangles::~Triangles()
{
}

void
Triangles::allocate_vertices(int nv)
{
  Geometry::allocate_vertices(nv);
  normals.resize(nv);
}


bool
Triangles::load_from_vtkPointSet(vtkPointSet *pset)
{
  if (pset->GetNumberOfPoints() == 0)
  {
    allocate(0, 0);
  }
  else
  {
    int nv = pset->GetNumberOfPoints();
    int npl = pset->GetNumberOfCells();

    allocate(nv, 3*npl);

    vtkFloatArray *parray = vtkFloatArray::SafeDownCast(pset->GetPoints()->GetData());
    if (! parray)
    {
      std::cerr << "can only handle float points\n";
      exit(1);
    }

    memcpy(vertices.data(), parray->GetVoidPointer(0), 3*nv*sizeof(float));

    vtkFloatArray *narray = vtkFloatArray::SafeDownCast(pset->GetPointData()->GetArray("Normals"));
    if (! narray)
      narray = vtkFloatArray::SafeDownCast(pset->GetPointData()->GetArray("Normals_"));

    if (narray)
      memcpy(normals.data(), narray->GetVoidPointer(0), 3*nv*sizeof(float));
    else
    {
      std::cerr << "triangle set has no normals\n";

      float *nptr = (float *)normals.data();
      for (int i = 0; i < nv; i++)
      {
        *nptr++ = 1.0;
        *nptr++ = 0.0;
        *nptr++ = 0.0;
      }
    }
    vtkDataArray *array = pset->GetPointData()->GetScalars();
    if (! array) array = pset->GetPointData()->GetArray("data");

    vtkFloatArray *farray = vtkFloatArray::SafeDownCast(array);
    if (farray)
      memcpy(data.data(), farray->GetVoidPointer(0), nv*sizeof(float));
    else
    {
      vtkDoubleArray *darray = vtkDoubleArray::SafeDownCast(array);
      double *ddata = (darray) ? (double *)darray->GetVoidPointer(0) : NULL;

      for (int i = 0; i < nv; i++)
        data[i] = (float)(ddata ? ddata[i] : 0.0);
    }

    int i = 0;
    for (vtkCellIterator *it = pset->NewCellIterator(); !it->IsDoneWithTraversal(); it->GoToNextCell())
    {
      vtkIdList *ids = it->GetPointIds();
      for (int j = 0; j < ids->GetNumberOfIds(); j++)
      {
        int id = ids->GetId(j);
        connectivity[i++] = id;
      }
    }
  }

  return true;
}

OsprayObjectP
Triangles::CreateTheOSPRayEquivalent(KeyedDataObjectP kdop)
{
  if (! ospData || hasBeenModified())
  {
    ospData = OsprayObject::Cast(OsprayTriangles::NewP(Triangles::Cast(kdop)));
    setModified(false);
  }

  return ospData;
}


} // namespace gxy
