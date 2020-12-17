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

#define CHECKBOX(XX)                                                            \
  for (vector<VisP>::iterator it = XX.begin(); it != XX.end(); it++)            \
  {                                                                              \
    VisP v = *it;                                                                \
                                                                                 \
    KeyedDataObjectP kdop = v->GetTheData();                                    \
    if (! kdop) std::cerr << "WARNING: NULL KeyedDataObjectP" << endl;          \
                                                                                \
    Box *l = kdop->get_local_box();                                              \
    Box *g = kdop->get_global_box();                                            \
                                                                                \
    if (first)                                                                  \
    {                                                                            \
      first = false;                                                            \
                                                                                \
      local_box = *l;                                                            \
      global_box = *g;                                                          \
                                                                                \
      for (int i = 0; i < 6; i++)                                                \
        neighbors[i] = kdop->get_neighbor(i);                                    \
    }                                                                            \
    else                                                                        \
    {                                                                            \
      if (local_box != *l || global_box != *g)                                  \
      {                                                                          \
        APP_PRINT(<< "ERROR: XXDatasets partitioning mismatch between participants:\n"    \
            << "\n" << "local xyz_min: " << local_box.xyz_min.x << " " << local_box.xyz_min.y << " " << local_box.xyz_min.z \
            << "\n" << "local xyz_max: " << local_box.xyz_max.x << " " << local_box.xyz_max.y << " " << local_box.xyz_max.z \
            << "\n" << "*l xyz_min: " << l->xyz_min.x << " " << l->xyz_min.y << " " << l->xyz_min.z \
            << "\n" << "*l xyz_max: " << l->xyz_max.x << " " << l->xyz_max.y << " " << l->xyz_max.z \
            << "\n" << "global xyz_min: " << global_box.xyz_min.x << " " << global_box.xyz_min.y << " " << global_box.xyz_min.z \
            << "\n" << "global xyz_max: " << global_box.xyz_max.x << " " << global_box.xyz_max.y << " " << global_box.xyz_max.z \
            << "\n" << "*g xyz_min: " << g->xyz_min.x << " " << g->xyz_min.y << " " << g->xyz_min.z \
            << "\n" << "*g xyz_max: " << g->xyz_max.x << " " << g->xyz_max.y << " " << g->xyz_max.z); \
        exit(1);                                                                \
      }                                                                          \
      for (int i = 0; i < 6; i++)                                                \
      {                                                                          \
        if (neighbors[i] != kdop->get_neighbor(i))                              \
         {                                                                      \
          APP_PRINT(<< "ERROR: Datasets partitioning mismatch between participants:\n"   \
                    << "N: " << neighbors[0] << " " \
                             << neighbors[1] << " " \
                             << neighbors[2] << " " \
                             << neighbors[3] << " " \
                             << neighbors[4] << " " \
                             << neighbors[5] << "\n"  \
                    << "K: " << kdop->get_neighbor(0) << " "  \
                             << kdop->get_neighbor(1) << " "  \
                             << kdop->get_neighbor(2) << " "  \
                             << kdop->get_neighbor(3) << " "  \
                             << kdop->get_neighbor(4) << " "  \
                             << kdop->get_neighbor(5) << "\n"); \
          exit(1);                                                              \
        }                                                                        \
      }                                                                          \
    }                                                                            \
  }

bool 
Visualization::local_commit(MPI_Comm c)
{
  bool first = true;

  for (auto v : vis)
    v->local_commit(c);


  CHECKBOX(vis)

#if 0
  std::cerr << "V " << std::hex << ((long)this) << std::dec << 
      " " << neighbors[0] << 
      " " << neighbors[1] << 
      " " << neighbors[2] << 
      " " << neighbors[3] << 
      " " << neighbors[4] << 
      " " << neighbors[5] << "\n";
#endif

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

  OSPModel ospModel = ospNewModel();

  void *mispc[vis.size()]; int nmispc = 0;
  void *vispc[vis.size()]; int nvispc = 0;

  for (auto v : vis)
  {
    KeyedDataObjectP kdop = v->GetTheData();
    OsprayObjectP op = v->GetTheOsprayDataObject();

    if (! op || kdop->hasBeenModified())
    {
      op = OsprayObject::Cast(kdop->CreateTheDeviceEquivalent(kdop));
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
          nvispc, vispc,
          nmispc, mispc,
          global_box.get_min(), global_box.get_max(),
          local_box.get_min(), local_box.get_max());
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
    + lighting.SerialSize();
}

unsigned char *
Visualization::serialize(unsigned char *p)
{
  p = lighting.Serialize(p);

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
  p = lighting.Deserialize(p);

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
