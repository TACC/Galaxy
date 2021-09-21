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

#include "Application.h"
#include "Renderer.h"

#include "Rendering.h"
#include "Visualization.h"
#include "Visualization_ispc.h"

#include <ospray/ospray.h>

#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"

using namespace rapidjson;

#include <map>

using namespace std;

namespace gxy
{

KEYED_OBJECT_CLASS_TYPE(Visualization)

void
Visualization::Register()
{
  RegisterClass();
}

void
Visualization::initialize()
{
  //std::cerr << "Visualization init " << std::hex << ((long)this) << "\n";
  ospModel = NULL;
  super::initialize();
}

Visualization::~Visualization()
{
  //std::cerr << "Visualization dtor " << std::hex << ((long)this) << "\n";
  Visualization::destroy_ispc();
}

void 
Visualization::allocate_ispc()
{
  ispc = ispc::Visualization_allocate();
}

void 
Visualization::initialize_ispc()
{
  ispc::Visualization_initialize(GetIspc());
}

bool
Visualization::Commit(DatasetsP datasets)
{
  for (auto s : vis)
    if (! s->Commit(datasets))
    {
      set_error(1);
      return false;
    }

  return KeyedObject::Commit();
}

bool
Visualization::Commit()
{
  for (auto s : vis)
    if (! s->Commit())
    {
      set_error(1);
      return false;
    }

  return KeyedObject::Commit();
}

vector<VisualizationP>
Visualization::LoadVisualizationsFromJSON(Value& v)
{
  vector<VisualizationP> visualizations;

  if (v.HasMember("Visualization") || v.HasMember("Visualizations"))
  {
    Value& c = v.HasMember("Visualization") ? v["Visualization"] : v["Visualizations"];
    if (c.IsArray())
    {
      for (int i = 0; i < c.Size(); i++)
      {
        VisualizationP v = Visualization::NewP();
        if (v->LoadFromJSON(c[i]))
          visualizations.push_back(v);
        else
          std::cerr << "error loading visualization " << i << "\n";
      }
    }
    else
    {
      VisualizationP v = Visualization::NewP();
      if (v->LoadFromJSON(c))
        visualizations.push_back(v);
      else
        std::cerr << "error loading visualization\n";
    }
  }

  return visualizations;
}

bool 
Visualization::local_commit(MPI_Comm c)
{
  bool first = true;

  for (auto v : vis)
    v->local_commit(c);

  return false;
}

void
Visualization::SetOsprayObjects(std::map<Key, OsprayObjectP>& ospray_object_map)
{
  if (! ispc)
  {
    allocate_ispc();
    initialize_ispc();
  }

  // Model for stuff that we'll be rtcIntersecting; lists of mappedvis and 
  // volumevis - NULL unless there's some model data

  ospModel = ospNewModel();

  void *mispc[vis.size()]; int nmispc = 0;
  void *vispc[vis.size()]; int nvispc = 0;

  for (auto v : vis)
  {
    KeyedDataObjectP kdop = v->GetTheData();

    OsprayObjectP op = kdop->CreateTheOSPRayEquivalent(kdop);
    if (! op)
      op = kdop->CreateTheOSPRayEquivalent(kdop);

    if (kdop->hasBeenModified() || !v->GetTheOsprayDataObject())
    {
      v->SetTheOsprayDataObject(op);
      kdop->setModified(false);
    }

    if (GeometryVis::IsA(v))
      ospAddGeometry(ospModel, (OSPGeometry)op->GetOSP());
    else
      vispc[nvispc++] = v->GetIspc();
  }

  if (ospModel)
    ospCommit(ospModel);
   
  ispc::Visualization_commit(ispc, 
          ospModel ? ospray_util::GetIE(ospModel) : NULL,
          nvispc, vispc, nmispc, mispc);
}

void
Visualization::AddVis(VisP o)
{
  vis.push_back(o);
}

bool 
Visualization::LoadFromJSON(Value& v)
{
  Value ops;

  if (v.HasMember("annotation"))
    SetAnnotation(string(v["annotation"].GetString()));

  if (v.HasMember("Lighting"))
    lighting.LoadStateFromValue(v["Lighting"]);

  else if (v.HasMember("lighting"))
    lighting.LoadStateFromValue(v["lighting"]);

  ops = v["operators"];
  for (int i = 0; i < ops.Size(); i++)
  {
    Value& vv = ops[i];

    if (! vv.HasMember("type"))
    {
      cerr << "ERROR: json has Visualization element with no type" << endl;
      exit(1);
    }

    string t = string(vv["type"].GetString());
    if (t.substr(t.size() - 3) != "Vis")
      t = t + "Vis";

    VisP vp = Vis::Cast(GetTheKeyedObjectFactory()->NewP(t));
    if (vp)
    {
      if (! vp->LoadFromJSON(vv))
      {
        set_error(1);
        return false;
      }
      AddVis(vp);
    }
    else
    {
      cerr << "ERROR: unknown type: " << " in json Visualization element (" << t << ")" << endl;
      set_error(1);
      return false;
    }
  }
  return true;
}

int 
Visualization::serialSize()
{
  return KeyedObject::serialSize() + (sizeof(int) + annotation.length() + 1) 
    + sizeof(int) + vis.size()*sizeof(Key)
    + lighting.serialSize();
}

unsigned char *
Visualization::serialize(unsigned char *p)
{
  p = lighting.serialize(p);

  int l = annotation.length() + 1;
  *(int *)p = l;
  p += sizeof(l);

  memcpy(p, annotation.c_str(), l-1);
  p[l-1] = 0;
  p += l;

  *(int *)p = vis.size();
  p += sizeof(int);

  for (auto v : vis)
  {
    *(Key *)p = v->getkey();
    p += sizeof(Key);
  }

  return p;
}

unsigned char *
Visualization::deserialize(unsigned char *p)
{
  p = lighting.deserialize(p);

  int l = *(int *)p;
  p += sizeof(l);
  annotation = (char *)p;
  p += l;

  int n = *(int *)p;
  p += sizeof(int);

  for (int i = 0; i < n; i++)
  {
    Key k = *(Key *)p;
    p += sizeof(Key);
    AddVis(Vis::GetByKey(k));
  }

  return p;
}

void 
Visualization::destroy_ispc()
{
  if (ispc)
  {
    ispc::Visualization_destroy(ispc);
  }
}

} // namespace gxy
