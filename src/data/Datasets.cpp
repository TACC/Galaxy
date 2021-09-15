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
#include <sstream>
#include <fstream>
#include <string>

#include "Datasets.h"

#include "Application.h"
#include "KeyedDataObject.h"
#include "Volume.h"
#include "Particles.h"
#include "Triangles.h"
#include "PathLines.h"

#include "rapidjson/rapidjson.h"

using namespace rapidjson;
using namespace std;

namespace gxy
{
  
KEYED_OBJECT_CLASS_TYPE(Datasets)

void
Datasets::Register()
{
  RegisterClass();
}

void
Datasets::initialize()
{
  keys = NULL;
  super::initialize();
}

Datasets::~Datasets()
{
  if (keys) delete[] keys;
}

bool
Datasets::Commit()
{
#if 0
  for (auto it : datasets)
    if (! it.second->Commit())
    {
      set_error(1);
      return false;
    }
#endif

  return KeyedObject::Commit();
}

void
Datasets::Insert(std::string name, KeyedDataObjectP val)
{
  if (datasets.find(name) != datasets.end())
  {
    // std::cerr << "replacing old datasets entry for " << name << "\n";
    datasets.erase(name);
  }

  datasets.insert(std::pair<std::string, KeyedDataObjectP>(name, val));

  DatasetsUpdate update(Added, name);
  NotifyObservers(gxy::ObserverEvent::Updated, (void *)&update);
}


bool
Datasets::loadTyped(PartitioningP p, Value& v)
{
  if (! v.HasMember("name") || ! v.HasMember("type"))
  {
    std::cerr << "Dataset must have name and type\n";
    set_error(1);
    return false;
  }

  string type(v["type"].GetString());

  KeyedDataObjectP kop;
  if (type == "Particles")
    kop = Particles::NewP();
  else if (type == "Volume")
    kop = Volume::NewP();
  else if (type == "Triangles")
    kop = Triangles::NewP();
  else if (type == "PathLines")
    kop = PathLines::NewP();
  else
  {
    std::cerr << "invalid Dataset type: " << type << "\n";
    set_error(1);
    return false;
  }

  if (! kop->LoadFromJSON(v, p))
    return false;

  kop->Commit();
    
  string name = v["name"].GetString();
  Insert(name.c_str(), kop);

  return true;
}

bool
Datasets::LoadFromJSON(Value& v, PartitioningP p)
{
  if (! v.HasMember("Datasets"))
  {
    std::cerr << "JSON has no Datasets clause\n";
    set_error(1);
    return false;
  }

  Value& ds = v["Datasets"];

  if (ds.IsArray())
  {
    for (int i = 0; i < ds.Size(); i++)
      if (! loadTyped(p, ds[i])) return false;
    return true;
  }
  else
    return loadTyped(p, ds);
}

bool
Datasets::LoadFromJSONFile(std::string fname, PartitioningP p)
{
  ifstream ifs(fname);
  if (! ifs)
  {
    std::cerr << "Unable to open " << fname << "\n";
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
      std::cerr << "JSON parse error in " << fname << "\n";
      set_error(1);
      return false;
    }

    return LoadFromJSON(doc, p);
  }
}

int
Datasets::serialSize()
{
  return KeyedObject::serialSize() + sizeof(int) + size()*sizeof(Key);
}

unsigned char * 
Datasets::serialize(unsigned char *p)
{
  *(int *)p = size();
  p += sizeof(int);

  for (auto i : datasets)
  {
    *(Key *)p = i.second->getkey();
    p += sizeof(Key);
  }

  return p;
}

unsigned char * 
Datasets::deserialize(unsigned char *p)
{
  nkeys = *(int *)p;
  p += sizeof(int);
  
  keys = new Key[nkeys];
  memcpy((void *)keys, (void *)p, nkeys*sizeof(Key));
  p += nkeys*sizeof(Key);

  return p;
}

}  // namespace gxy
