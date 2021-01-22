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
#include "vtkerror.h"
#include "PathLines.h"
// #include "EmbreePathLines.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>

#include <string>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <vtkCell.h>
#include <vtkCellType.h>
#include <vtkCellIterator.h>
#include <vtkDataArray.h>
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

OBJECT_CLASS_TYPE(PathLines) 

void     
PathLines::Register()
{        
  RegisterClass();
}

void
PathLines::initialize()
{
  super::initialize();
};

PathLines::~PathLines()
{
}

bool
PathLines::load_from_vtkPointSet(vtkPointSet *pset)
{
  if (pset->GetNumberOfPoints() == 0)
  {
    allocate(0, 0);
  }
  else
  {
    int nv = pset->GetNumberOfPoints();
    int npl = pset->GetNumberOfCells();

    int k = 0;
    for (vtkCellIterator *it = pset->NewCellIterator(); !it->IsDoneWithTraversal(); it->GoToNextCell())
      k = k + it->GetNumberOfPoints();

    allocate(k, k - pset->GetNumberOfCells());

    vtkFloatArray *parray = vtkFloatArray::SafeDownCast(pset->GetPoints()->GetData());
    if (! parray) 
    {
      std::cerr << "can only handle float points\n";
      exit(1);
    }

    vec3f *points = (vec3f *)parray->GetVoidPointer(0);

    vtkDataArray *array = pset->GetPointData()->GetScalars();
    if (! array) array = pset->GetPointData()->GetArray("data");

    vtkFloatArray *farray = vtkFloatArray::SafeDownCast(array);
    float *fdata = (farray) ? (float *)farray->GetVoidPointer(0) : NULL;

    vtkDoubleArray *darray = vtkDoubleArray::SafeDownCast(array);
    double *ddata = (darray) ? (double *)darray->GetVoidPointer(0) : NULL;

    int i = 0, j = 0; k = 0;
    for (vtkCellIterator *it = pset->NewCellIterator(); !it->IsDoneWithTraversal(); it->GoToNextCell(), i++)
    {
      vtkIdList *ids = it->GetPointIds();
      for (int l = 0; l < ids->GetNumberOfIds(); l++, k++)
      {
        int id = ids->GetId(l);
        (*vertices)[k] = points[id];
        (*data)[k] = (fdata) ? fdata[id] : (ddata) ? ((float)ddata[id]) : 0.0;
        if (l < (ids->GetNumberOfIds() - 1))
          (*connectivity)[j++] = k;
      }
    }
  }

  return true;
}

void
PathLines::GetPLVertices(PLVertex*& p, int& n)
{
  n = vertices->size();
  p = new PLVertex[n];
  for (int i = 0; i < n; i++)
  {
    p[i].xyz = (*vertices)[i];
    p[i].value = (*data)[i];
  }
}

bool 
PathLines::CreateTheDeviceEquivalent(KeyedDataObjectDPtr kdop)
{ 
  // return GalaxyObject::DCast(OsprayPathLines::NewDistributed(PathLines::DCast(kdop)));
  return false;
} 


} // namespace gxy
