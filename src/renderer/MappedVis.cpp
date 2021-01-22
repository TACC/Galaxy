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
#include <math.h>
#include <stdlib.h>
#include <string>
#include <sstream>
#include <fstream>
#include "Application.h"
#include "MappedVis.h"

#include "MappedVis_ispc.h"

#include "rapidjson/document.h"

using namespace std;
using namespace rapidjson;

namespace gxy
{

OBJECT_CLASS_TYPE(MappedVis)

void
MappedVis::Register()
{
  RegisterClass();
}

MappedVis::~MappedVis()
{
}

void
MappedVis::initialize()
{
  super::initialize();

  colormap.push_back(vec4f(0.0, 0.4, 0.4, 0.4));
  colormap.push_back(vec4f(1.0, 1.0, 1.0, 1.0));

  opacitymap.push_back(vec2f(0.0, 1.0));
  opacitymap.push_back(vec2f(1.0, 1.0));
}

void 
MappedVis::initialize_ispc()
{
  super::initialize_ispc();

  ::ispc::MappedVis_ispc *myspc = (::ispc::MappedVis_ispc*)ispc;
  myspc->colorMap   = color_tf.GetIspc();
  myspc->opacityMap = opacity_tf.GetIspc();
}

void
MappedVis::allocate_ispc()
{
  ispc = malloc(sizeof(::ispc::MappedVis_ispc));
}

bool 
MappedVis::Commit(DatasetsDPtr datasets)
{
  return Vis::Commit(datasets);
}

bool
MappedVis::LoadFromJSON(Value& v)
{
  Vis::LoadFromJSON(v);

  if (v.HasMember("data range"))
    {
      data_range_min = v["data range"][0].GetDouble();
      data_range_max = v["data range"][1].GetDouble();
      data_range = true;
    }
    else
    {
        data_range = false;
    }

  if (v.HasMember("transfer function") || v.HasMember("colormap"))
  {
    const Value& m = v.HasMember("transfer function") ? v["transfer function"] : v["colormap"];

    if (m.IsString())
    {
      string fname = m.GetString();
      
      if (fname.length() != 0 && m != "default")
      {
        ifstream ifs(fname);
        if (! ifs)
        {
          std::cerr << "unable to open transfer function file: " << fname << "\n";
          set_error(1);
          return false;
        }
    
        stringstream ss;
        ss << ifs.rdbuf();

        string s(ss.str().substr(ss.str().find('[')+1, ss.str().rfind(']') - 2));

        Document doc;
        if (doc.Parse<0>(ss.str().c_str()).HasParseError())
        {
          std::cerr << "JSON parse error in " << fname << "\n";
          set_error(1);
          return false;
        }

        doc.Parse(s.c_str());

        Value& cmap = doc.IsArray() ? doc[0] : doc;

        opacitymap.clear();

        if (cmap.HasMember("Points"))
        {
          Value& oa = cmap["Points"];
          for (int i = 0; i < oa.Size(); i += 4)
          {
            vec2f xo;
            xo.x = oa[i+0].GetDouble();
            xo.y = oa[i+2].GetDouble();
            opacitymap.push_back(xo);
          }
        }
        else
        {
          vec2f xo = {0.0, 1.0};
          opacitymap.push_back(xo);
          xo = {1.0, 1.0};
          opacitymap.push_back(xo);
        }

        colormap.clear();

        Value& rgba = cmap["RGBPoints"];
        for (int i = 0; i < rgba.Size(); i += 4)
        {
          vec4f xrgb;
          xrgb.x = rgba[i+0].GetDouble();
          xrgb.y = rgba[i+1].GetDouble();
          xrgb.z = rgba[i+2].GetDouble();
          xrgb.w = rgba[i+3].GetDouble();
          colormap.push_back(xrgb);
        }
      }
    }
    else
    {
      colormap.clear();

      for (int i = 0; i < m.Size(); i++)
      {
        vec4f xrgb;
        xrgb.x = m[i][0].GetDouble();
        xrgb.y = m[i][1].GetDouble();
        xrgb.z = m[i][2].GetDouble();
        xrgb.w = m[i][3].GetDouble();
        colormap.push_back(xrgb);
      }

      if (v.HasMember("opacitymap"))
      {
        opacitymap.clear();

        Value& om = v["opacitymap"];
        for (int i = 0; i < om.Size(); i++)
        {
          vec2f xo;
          xo.x = om[i][0].GetDouble();
          xo.y = om[i][1].GetDouble();
          opacitymap.push_back(xo);
        }
      }
    }
  }

  return true;
}

int
MappedVis::serialSize() 
{
  return super::serialSize() + sizeof(Key) +
         sizeof(int) + colormap.size()*sizeof(vec4f) +
         sizeof(int) + opacitymap.size()*sizeof(vec2f) +
                 sizeof(float) + sizeof(float) + sizeof(bool);
}

unsigned char *
MappedVis::deserialize(unsigned char *ptr) 
{
  ptr = super::deserialize(ptr);

  int nc = *(int *)ptr;
  ptr += sizeof(int);
  SetColorMap(nc, (vec4f *)ptr);
  ptr += nc * sizeof(vec4f);

  int no = *(int *)ptr;
  ptr += sizeof(int);
  SetOpacityMap(no, (vec2f *)ptr);
  ptr += no * sizeof(vec2f);

  data_range_min = *(float *)ptr;
  ptr += sizeof(float);

  data_range_max = *(float *)ptr;
  ptr += sizeof(float);

  data_range = *(bool *)ptr;
  ptr += sizeof(bool);

  return ptr;
}

unsigned char *
MappedVis::serialize(unsigned char *ptr)
{
  ptr = super::serialize(ptr);

  *(int *)ptr = colormap.size();
  ptr += sizeof(int);
  memcpy(ptr, colormap.data(), colormap.size()*sizeof(vec4f));
  ptr += colormap.size()*sizeof(vec4f);

  *(int *)ptr = opacitymap.size();
  ptr += sizeof(int);
  memcpy(ptr, opacitymap.data(), opacitymap.size()*sizeof(vec2f));
  ptr += opacitymap.size()*sizeof(vec2f);

  *(float *)ptr = data_range_min;
  ptr += sizeof(float);

  *(float *)ptr = data_range_max;
  ptr += sizeof(float);

  *(bool *)ptr = data_range; 
  ptr += sizeof(bool);

  return ptr;
}

bool 
MappedVis::local_commit(MPI_Comm c)
{
  if(super::local_commit(c))  
    return true;

  color_tf.Interpolate(colormap.size(), (float *)colormap.data());
  color_tf.SetMinMax(data_range_min, data_range_max);

  opacity_tf.Interpolate(opacitymap.size(), (float *)opacitymap.data());
  opacity_tf.SetMinMax(data_range_min, data_range_max);

  return false;
}

void 
MappedVis::SetColorMap(int n, vec4f *ptr)
{
  colormap.clear();
  for (int i = 0; i < n; i++)
    colormap.push_back(ptr[i]);
}      
       
void 
MappedVis::SetOpacityMap(int n, vec2f *ptr)
{      
  opacitymap.clear();
  for (int i = 0; i < n; i++)
    opacitymap.push_back(ptr[i]);
}

void
MappedVis::ScaleMaps(float xmin, float xmax)
{
  float x0 = colormap[0].x;
  float x1 = colormap[colormap.size()-1].x;

  for (auto i = 0; i < colormap.size(); i++)
    colormap[i].x = xmin + ((colormap[i].x - x0)/(x1 - x0)) * (xmax - xmin);

  x0 = opacitymap[0].x;
  x1 = opacitymap[opacitymap.size()-1].x;

  for (auto i = 0; i < opacitymap.size(); i++)
    opacitymap[i].x = xmin + ((opacitymap[i].x - x0)/(x1 - x0)) * (xmax - xmin);
}


} // namespace gxy
