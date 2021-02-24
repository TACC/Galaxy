// ========================================================================== //
//                                                                            //
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
#include <string>
#include <fstream>
#include <sstream>

#include <Application.h>
#include <KeyedObject.h>
#include <Geometry.h>

#include "rapidjson/document.h"

namespace gxy
{

OBJECT_POINTER_TYPES(Partitioning)

class Partitioning :  public gxy::KeyedObject
{
  KEYED_OBJECT(Partitioning)

public:
  struct Extent
  {
    float xmin, xmax;
    float ymin, ymax;
    float zmin, zmax;

    bool IsIn(float *p, float fuzz)
    {
      if (p[0] < xmin - fuzz || p[0] > xmax + fuzz) return false;
      if (p[1] < ymin - fuzz || p[1] > ymax + fuzz) return false;
      if (p[2] < zmin - fuzz || p[2] > zmax + fuzz) return false;
      return true;
    }
  };

  bool Load(std::string);
  bool Gather(GeometryP);

  int number_of_partitions() { return n_parts; }
  Extent *get_extent(int i) { return extents + i; }
  Extent *get_extents() { return extents; }

  void SetExtents(int n, Extent *e)
  {
    if (extents) free(extents);
    n_parts = n;
    extents = (Extent *)malloc(n*sizeof(Extent));
    memcpy(extents, e, n*sizeof(Extent));
  }

  bool IsIn(int i, float *p, float fuzz)
  {
    return extents[i].IsIn(p, fuzz);
  }

    
private:
  Extent *extents = NULL;
  int n_parts = -1;

  class GatherMsg : public Work
  {
    struct GatherMsgArgs
    {
      Key pk; // Partitioning object
      Key gk; // Geometry object
    };

  public:
    GatherMsg(Partitioning* p, GeometryP g) : GatherMsg(sizeof(struct GatherMsgArgs))
    {
      struct GatherMsgArgs *a = (struct GatherMsgArgs *)contents->get();
      a->pk = p->getkey();
      a->gk = g->getkey();
    }

    WORK_CLASS(GatherMsg, true);

    bool CollectiveAction(MPI_Comm c, bool isRoot);
  };

  class DistributeMsg : public Work
  {
    struct DistributeMsgArgs
    {
      Key pk;     // Partitioning object
      int nparts;
      Extent extents[];
    };

  public:
    DistributeMsg(Partitioning* p) : 
                DistributeMsg(sizeof(struct DistributeMsgArgs) + p->number_of_partitions()*sizeof(Extent))
    {
      struct DistributeMsgArgs *a = (struct DistributeMsgArgs *)contents->get();
      a->pk = p->getkey();
      a->nparts = p->number_of_partitions();
      memcpy(a->extents, p->extents, p->number_of_partitions()*sizeof(Extent));
    }

    WORK_CLASS(DistributeMsg, true);

    bool CollectiveAction(MPI_Comm c, bool isRoot);
  };
};

}
