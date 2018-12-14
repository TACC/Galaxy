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

#include <iostream>

#include "IF.h"
using namespace gxy;

#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"

using namespace rapidjson;

//========= Datasets

void
DatasetsIF::Add(std::string n, std::string t, std::string filename)
{
  datasets.push_back(Dataset(n, t, filename));
}

void
DatasetsIF::Load(Value& v)
{
  if (v.HasMember("Datasets"))
  {
    Value& ds = v["Datasets"];
    for (SizeType i = 0; i < ds.Size(); i++)
      Add(ds[i]["name"].GetString(), ds[i]["type"].GetString(), ds[i]["filename"].GetString());
  }
}

char *
DatasetsIF::Stringify()
{
  Document doc;
  doc.Parse("{}");

  Value a(kArrayType);
  for (std::vector<Dataset>::iterator it = datasets.begin(); it != datasets.end(); ++it)
  {
    Value d(kObjectType);
    d.AddMember("name", Value().SetString(it->name.c_str(), it->name.size()+1), doc.GetAllocator());
    d.AddMember("type", Value().SetString(it->type.c_str(), it->type.size()+1), doc.GetAllocator());
    d.AddMember("filename", Value().SetString(it->filename.c_str(), it->filename.size()+1), doc.GetAllocator());
    a.PushBack(d, doc.GetAllocator());
  }

  doc.AddMember("Datasets", a, doc.GetAllocator());

  StringBuffer buffer;
  Writer<StringBuffer> writer(buffer);
  doc.Accept(writer);

  char *buf = (char *)malloc(strlen(buffer.GetString()) + 6);
  strcpy(buf, "json ");
  strcat(buf, buffer.GetString());

  return buf;
}

void
DatasetsIF::Print()
{
  for (std::vector<Dataset>::iterator it = datasets.begin(); it != datasets.end(); ++it)
    std::cerr << it->name << " " << it->type << " " << it->filename << "\n";
}

//========= Camera
    
void
CameraIF::Load(Value& v)
{
  if (v.HasMember("Camera") || v.HasMember("Cameras"))
  {
    Value& c = v.HasMember("Camera") ? v["Camera"] : v["Cameras"];
    if (c.IsArray())
      c = c[0];

    eye[0] = c["viewpoint"][0].GetDouble();
    eye[1] = c["viewpoint"][1].GetDouble();
    eye[2] = c["viewpoint"][2].GetDouble();

    if (c.HasMember("viewdirection"))
    {
      dir[0] = c["viewdirection"][0].GetDouble();
      dir[1] = c["viewdirection"][1].GetDouble();
      dir[2] = c["viewdirection"][2].GetDouble();
    }
    else if (c.HasMember("viewcenter"))
    {
      dir[0] = c["viewcenter"][0].GetDouble() - eye[0];
      dir[1] = c["viewcenter"][1].GetDouble() - eye[1];
      dir[2] = c["viewcenter"][2].GetDouble() - eye[2];
    }
    else
    {
        std::cerr << "need either viewdirection or viewcenter\n";
        exit(1);
    }

    up[0] = c["viewup"][0].GetDouble();
    up[1] = c["viewup"][1].GetDouble();
    up[2] = c["viewup"][2].GetDouble();

    aov = c["aov"].GetDouble();
  }
}

char *
CameraIF::Stringify()
{
  Document doc;
  doc.Parse("{}");

  Value c(kObjectType);

  Value e(kArrayType);
  e.PushBack(Value().SetDouble(eye[0]), doc.GetAllocator());
  e.PushBack(Value().SetDouble(eye[1]), doc.GetAllocator());
  e.PushBack(Value().SetDouble(eye[2]), doc.GetAllocator());
  c.AddMember("viewpoint", e, doc.GetAllocator());

  Value d(kArrayType);
  d.PushBack(Value().SetDouble(dir[0]), doc.GetAllocator());
  d.PushBack(Value().SetDouble(dir[1]), doc.GetAllocator());
  d.PushBack(Value().SetDouble(dir[2]), doc.GetAllocator());
  c.AddMember("viewdirection", d, doc.GetAllocator());

  Value u(kArrayType);
  u.PushBack(Value().SetDouble(up[0]), doc.GetAllocator());
  u.PushBack(Value().SetDouble(up[1]), doc.GetAllocator());
  u.PushBack(Value().SetDouble(up[2]), doc.GetAllocator());
  c.AddMember("viewup", u, doc.GetAllocator());

  c.AddMember("aov", Value().SetDouble(aov), doc.GetAllocator());

  doc.AddMember("Camera", c, doc.GetAllocator());

  StringBuffer buffer;
  Writer<StringBuffer> writer(buffer);
  doc.Accept(writer);

  char *buf = (char *)malloc(strlen(buffer.GetString()) + 6);
  strcpy(buf, "json ");
  strcat(buf, buffer.GetString());

  return buf;
}

void
CameraIF::Print()
{
  std::cerr << "eye: " << eye[0] << " " << eye[1] << " " << eye[2] << "\n";
  std::cerr << "dir: " << dir[0] << " " << dir[1] << " " << dir[2] << "\n";
  std::cerr << "up: "  << up[0] << " " << up[1] << " " << up[2] << "\n";
  std::cerr << "aov: " << aov << "\n";
}
    
