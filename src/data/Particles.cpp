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
#include <vtkCell.h>
#include <vtkCellType.h>
#include <vtkCellIterator.h>
#include <vtkSmartPointer.h>
#include <vtkDataSetReader.h>
#include <vtkDataArray.h>
#include <vtkDoubleArray.h>
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

KEYED_OBJECT_CLASS_TYPE(Particles)

void
Particles::Register()
{
  RegisterClass();
}

void
Particles::initialize()
{
  super::initialize();
}

Particles::~Particles()
{
}

bool
Particles::local_commit(MPI_Comm c)
{
  struct part
  {
    int n;
    float m, M;
  };

  part mine, parts[GetTheApplication()->GetSize()];

  mine.n = data.size();
  mine.m = 0;
  mine.M = 0;

  float gmin = 0, gmax = 0;

  if (data.size() != 0)
  {
    float *dptr = (float *)data.data();

    mine.m = *dptr;
    mine.M = *dptr;
    dptr++;

    for (int i = 1; i < data.size(); i++)
    {
      float v = *dptr++;
      if (v > mine.M) mine.M = v;
      if (v < mine.M) mine.m = v;
    }
    
    if (GetTheApplication()->GetTheMessageManager()->UsingMPI())
    {
      MPI_Allgather(&mine, sizeof(mine), MPI_UNSIGNED_CHAR, parts, GetTheApplication()->GetSize()*sizeof(mine), MPI_UNSIGNED_CHAR, c);

      for (int i = 0, first = 1; i < GetTheApplication()->GetSize(); i++)
        if (parts[i].n > 0)
        {
          if (first)
          {
            first = 0;
            gmin = parts[i].m;
            gmax = parts[i].M;
          }
          else
          {
            gmin = (gmin < parts[i].m) ? gmin : parts[i].m;
            gmax = (gmax < parts[i].M) ? gmax : parts[i].M;
          }
        }
    }
    else
    {
      gmin = mine.m;
      gmax = mine.M;
    }
  }

  set_local_minmax(mine.m, mine.M);
  set_global_minmax(gmin, gmax);

  return false;
}

void
Particles::GetParticles(Particle*& p, int& n)
{
  n = vertices.size();
  p = new Particle[n];
  for (int i = 0; i < n; i++)
  { 
    p[i].xyz = vertices[i];
    p[i].u.value = data[i];
  }
}

bool
Particles::load_from_vtkPointSet(vtkPointSet *pset)
{
  if (pset->GetNumberOfPoints() == 0)
  {
    allocate(0, 0);
  }
  else
  {
    int nv = pset->GetNumberOfPoints();

    allocate(nv, 0);

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

    int i = 0;
    for (int i = 0; i < nv; i++)
    {
      vertices[i] = points[i];
      data[i] = (fdata) ? fdata[i] : (ddata) ? ((float)ddata[i]) : 0.0;
    }
  }

  return true;
}


} // namespace gxy
