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

#include <string>
#include <string.h>
#include <memory.h>
#include <vector>

#include "Geometry.h"
#include "Particles.h"
#include "Triangles.h"
#include "PathLines.h"

#include "rapidjson/filereadstream.h"

using namespace std;
using namespace rapidjson;


namespace gxy
{
KEYED_OBJECT_CLASS_TYPE(Geometry)

void
Geometry::Register()
{
	RegisterClass();
	Particles::Register();
	Triangles::Register();
	PathLines::Register();
}

bool
Geometry::get_partitioning_from_file(char *s)
{
  ifstream ifs(s);
  if (! ifs)
  {
    std::cerr << "Unable to open " << s << "\n";
    set_error(1);
    return false;
  }
  else
  {
    stringstream ss;
    ss << ifs.rdbuf();

    Document doc;
    if (doc.Parse<0>(ss.str().c_str()).HasParseError())
    {
      std::cerr << "JSON parse error in " << s << "\n";
      set_error(1);
      return false;
    }

    return get_partitioning(doc);
  }
}

bool
Geometry::get_partitioning(Value& doc)
{
  if (!doc.HasMember("parts"))
  {
    cerr << "partition document does not have parts\n";
    set_error(1);
    return false;
  }

  Value& parts = doc["parts"];

  if (!parts.IsArray() || parts.Size() != GetTheApplication()->GetSize())
  {
    cerr << "invalid partition document\n";
    set_error(1);
    return false;
  }

  float extents[6*parts.Size()];

  for (int i = 0; i < parts.Size(); i++)
  {
    Value& ext = parts[i]["extent"];
    for (int j = 0; j < 6; j++)
      extents[i*6 + j] = ext[j].GetDouble();
  }

  for (int i = 0; i < 6; i++) neighbors[i] = -1;

  float *extent = extents + 6*GetTheApplication()->GetRank();
	float gminmax[6], lminmax[6];

  gminmax[0] =  FLT_MAX;
  gminmax[1] = -FLT_MAX;
  gminmax[2] =  FLT_MAX;
  gminmax[3] = -FLT_MAX;
  gminmax[4] =  FLT_MAX;
  gminmax[5] = -FLT_MAX;

  memcpy(lminmax, extents + 6*GetTheApplication()->GetRank(), 6*sizeof(float));

  for (int i = 0; i < parts.Size(); i++)
  {
    float *e = extents + i*6;

#define LAST(axis) (e[2*axis + 1] == lminmax[2*axis + 0])
#define NEXT(axis) (e[2*axis + 0] == lminmax[2*axis + 1])
#define EQ(axis)   (e[2*axis + 0] == lminmax[2*axis + 0])

    if (e[0] < gminmax[0]) gminmax[0] = e[0];
    if (e[1] > gminmax[1]) gminmax[1] = e[1];
    if (e[2] < gminmax[2]) gminmax[2] = e[2];
    if (e[3] > gminmax[3]) gminmax[3] = e[3];
    if (e[4] < gminmax[4]) gminmax[4] = e[4];
    if (e[5] > gminmax[5]) gminmax[5] = e[5];

    if (LAST(0) && EQ(1) && EQ(2)) neighbors[0] = i;
    if (NEXT(0) && EQ(1) && EQ(2)) neighbors[1] = i;

    if (EQ(0) && LAST(1) && EQ(2)) neighbors[2] = i;
    if (EQ(0) && NEXT(1) && EQ(2)) neighbors[3] = i;

    if (EQ(0) && EQ(1) && LAST(2)) neighbors[4] = i;
    if (EQ(0) && EQ(1) && NEXT(2)) neighbors[5] = i;
  }

	global_box = Box(vec3f(gminmax[0], gminmax[2], gminmax[4]), vec3f(gminmax[1], gminmax[3], gminmax[5]));
	local_box = Box(vec3f(lminmax[0], lminmax[2], lminmax[4]), vec3f(lminmax[1], lminmax[3], lminmax[5]));

  return true;
}


} // namespace gxy
