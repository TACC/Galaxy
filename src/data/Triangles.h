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

#pragma once

#include <string>
#include <string.h>
#include <memory.h>

using namespace std;

#include "KeyedDataObject.h"
#include "dtypes.h"
#include "Application.h"
#include "Box.h"
#include "Geometry.h"
#include "Datasets.h"
using namespace std;

#include "rapidjson/document.h"

#include "ospray/ospray.h"

namespace pvol
{

KEYED_OBJECT_POINTER(Triangles)

class Triangles : public Geometry
{
  KEYED_OBJECT_SUBCLASS(Triangles, Geometry)

public:
	void initialize();
	virtual ~Triangles();

  virtual bool local_commit(MPI_Comm);
  virtual void local_import(char *, MPI_Comm);

  virtual void LoadFromJSON(Value&);
  virtual void SaveToJSON(Value&, Document&);

private:
	int n_triangles;
	int n_vertices;
	float *vertices;
	float *normals;
	int *triangles;
};

}
