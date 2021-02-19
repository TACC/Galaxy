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

#include "Partitioning.hpp"

// #include <stdio.h>
#include <stdlib.h>

namespace gxy {

WORK_CLASS_TYPE(Partitioning::GatherMsg)
WORK_CLASS_TYPE(Partitioning::DistributeMsg)
KEYED_OBJECT_CLASS_TYPE(Partitioning)

void
Partitioning::Register()
{
  RegisterClass();
  Partitioning::GatherMsg::Register();
  Partitioning::DistributeMsg::Register();
}

bool
Partitioning::Load(std::string s)
{
  std::ifstream ifs(s);
  if (! ifs.good())
  { 
    std::cerr << "error opening file\n";
    return false;
  }

  std::stringstream ss;
  ss << ifs.rdbuf();

  rapidjson::Document doc;
  if (doc.Parse<0>(ss.str().c_str()).HasParseError())
  {
    std::cerr << "JSON parse error in " << s << "\n";
    return false;
  }

  if (! doc.HasMember("parts"))
  {
    std::cerr << "JSON document has no extents\n";
    exit(1);
  }

  rapidjson::Value& parts = doc["parts"];
  if (! parts.IsArray())
  {
    std::cerr << "parts section is not an array\n";
    exit(1);
  }

  n_parts = parts.Size();

  extents = new Extent[n_parts];

  for (int i = 0; i < n_parts; i++)
  {
    Extent& extent = extents[i];
    rapidjson::Value& part = parts[i];

    if (! part.HasMember("extent"))
    {
      std::cerr << "partition has no extent\n";
      exit(1);
    }

    rapidjson::Value& e = part["extent"];
    if (! e.IsArray() || e.Size() != 6)
    {
      std::cerr << "extent is not an array or not 6 in length\n";
      return false;
    }

    extent.xmin = e[0].GetDouble();
    extent.xmax = e[1].GetDouble();
    extent.ymin = e[2].GetDouble();
    extent.ymax = e[3].GetDouble();
    extent.zmin = e[4].GetDouble();
    extent.zmax = e[5].GetDouble();
  }

  DistributeMsg msg(this);
  msg.Broadcast(true, true);
 
  return true;
}

bool
Partitioning::Gather(GeometryP g)
{
  GatherMsg msg(this, g);
  msg.Broadcast(true, true);
  return true;
}

bool
Partitioning::GatherMsg::CollectiveAction(MPI_Comm c, bool isRoot)
{
  struct GatherMsgArgs *p = (struct GatherMsgArgs *)get();

  PartitioningP partitioning = Partitioning::GetByKey(p->pk);
  GeometryP geometry = Geometry::GetByKey(p->gk);

  Box *lbox = geometry->get_local_box();
  Extent lextent;

  lbox->get_min(lextent.xmin, lextent.ymin, lextent.zmin);
  lbox->get_max(lextent.xmax, lextent.ymax, lextent.zmax);

  if (partitioning->extents) free(partitioning->extents);
  partitioning->n_parts = GetTheApplication()->GetSize();
  partitioning->extents = (Extent *)malloc(partitioning->n_parts * sizeof(Extent));

  MPI_Allgather(&lextent, sizeof(lextent), MPI_CHAR, 
                partitioning->extents, sizeof(lextent), MPI_CHAR, c);

  return false;
}

bool
Partitioning::DistributeMsg::CollectiveAction(MPI_Comm c, bool isRoot)
{
  struct DistributeMsgArgs *a = (struct DistributeMsgArgs *)get();

  if (! isRoot)
  {
    PartitioningP partitioning = Partitioning::GetByKey(a->pk);
    partitioning->SetExtents(a->nparts, a->extents);
  }
  
  return false;
}

}

