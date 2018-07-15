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

#include "Visualization.h"
#include "Visualization_ispc.h"

#include "Application.h"
#include "OSPUtil.h"

#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"

using namespace rapidjson;
using namespace std;

namespace gxy
{

KEYED_OBJECT_TYPE(Visualization)

void
Visualization::Register()
{
  RegisterClass();
}

void
Visualization::initialize()
{
	ospModel = NULL;
	super::initialize();
}

Visualization::~Visualization()
{
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
  ispc::Visualization_initialize(GetISPC());
}

void
Visualization::Drop()
{
	while(! volumes.empty())
	{
		volumes.back()->Drop();
		volumes.pop_back();
	}

	while(! mappedGeometries.empty())
	{
		mappedGeometries.back()->Drop();
		mappedGeometries.pop_back();
	}

	while(! osprayGeometries.empty())
	{
		osprayGeometries.back()->Drop();
		osprayGeometries.pop_back();
	}

	KeyedObject::Drop();
}

void
Visualization::Commit(DatasetsP datasets)
{
	for (auto s : volumes)
		s->Commit(datasets);

	for (auto s : mappedGeometries)
		s->Commit(datasets);

	for (auto s : osprayGeometries)
		s->Commit(datasets);

	KeyedObject::Commit();
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
        v->LoadFromJSON(c[i]);
        visualizations.push_back(v);
			}
    }
    else
		{
			VisualizationP v = Visualization::NewP();
			v->LoadFromJSON(c);
			visualizations.push_back(v);
		}
  }
  else
    std::cerr << "No visualizations found\n";

  return visualizations;
}

#define CHECKBOX(XX)																														\
	for (vector<VisP>::iterator it = XX.begin(); it != XX.end(); it++)					  \
	{																																						  \
		VisP v = *it;																															  \
	 																																						  \
		KeyedDataObjectP kdop = v->GetTheData();																	  \
		if (! kdop) std::cerr << "WARNING: NULL KeyedDataObjectP" << endl;					\
																																							  \
		Box *l = kdop->get_local_box();																						  \
		Box *g = kdop->get_global_box();																					  \
																																							  \
		if (first)																																  \
		{																																					  \
			first = false;																													  \
																																							  \
      local_box = *l;																													  \
      global_box = *g;																												  \
																																							  \
      for (int i = 0; i < 6; i++)																							  \
        neighbors[i] = kdop->get_neighbor(i);																	  \
    }																																					  \
    else																																			  \
    {																																					  \
      if (local_box != *l || global_box != *g)																  \
      {																																				  \
        APP_PRINT(<< "ERROR: Datasets partitioning mismatch between participants");	  \
        exit(1);																															  \
      }																																				  \
      for (int i = 0; i < 6; i++)																							  \
      {																																				  \
        if (neighbors[i] != kdop->get_neighbor(i))														  \
        {																																			  \
          APP_PRINT(<< "ERROR: Datasets partitioning mismatch between participants");	\
          exit(1);																															\
        }																																				\
      }																																					\
    }																																						\
	}

bool 
Visualization::local_commit(MPI_Comm c)
{
	bool first = true;

  if (! ispc)
	{
    allocate_ispc();
		initialize_ispc();
	}

	CHECKBOX(osprayGeometries)
	CHECKBOX(mappedGeometries)
	CHECKBOX(volumes)

   
	// Model for stuff that we'll be rtcIntersecting

	if (ospModel) ospRelease(ospModel);
	ospModel = ospNewModel();

  for (auto ospg : osprayGeometries)
		if (ospg->GetTheData()->GetOSP() != nullptr)
			ospAddGeometry(ospModel, (OSPGeometry)ospg->GetTheData()->GetOSP());
  
  ospCommit(ospModel);
   
  //cerr << "volumes: " << volumes.size() <<  " " <<  volumes.data() << endl;
  //cerr << "mappedGeometries: " << mappedGeometries.size() <<  " " <<  mappedGeometries.data() << endl;
  
  void *mispc[mappedGeometries.size()+1];   
  for (int i = 0; i < mappedGeometries.size(); i++)
    mispc[i] = mappedGeometries[i]->GetISPC();
  
  void *vispc[volumes.size()+1];
  for (int i = 0; i < volumes.size(); i++)
    vispc[i] = volumes[i]->GetISPC();
  
  ispc::Visualization_commit(ispc, 
          osp_util::GetIE(ospModel),
          volumes.size(), vispc,
          mappedGeometries.size(), mispc,
          global_box.get_min(), global_box.get_max(),
          local_box.get_min(), local_box.get_max());
  
	return false;
}

void
Visualization::AddMappedGeometryVis(VisP o)
{
	mappedGeometries.push_back(o);
}

void
Visualization::AddOsprayGeometryVis(VisP o)
{
	osprayGeometries.push_back(o);
}

void
Visualization::AddVolumeVis(VisP o)
{
	volumes.push_back(o);
}

void 
Visualization::SaveToJSON(Value& v, Document&  doc)
{
	Value varray(kArrayType);
	for (auto it : mappedGeometries)
	{
		Value v(kObjectType);
		it->SaveToJSON(v, doc);
		varray.PushBack(v, doc.GetAllocator());
	}
	for (auto it : osprayGeometries)
	{
		Value v(kObjectType);
		it->SaveToJSON(v, doc);
		varray.PushBack(v, doc.GetAllocator());
	}
	for (auto it : volumes)
	{
		Value v(kObjectType);
		it->SaveToJSON(v, doc);
		varray.PushBack(v, doc.GetAllocator());
	}

	v.AddMember("Visualization", varray, doc.GetAllocator());
}

void 
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

		VisP vp;
		if (t == "Volume")
		{
			VisP vp = VolumeVis::NewP();
			vp->LoadFromJSON(vv);
			AddVolumeVis(vp);
		}
		else if (t == "Particles")
		{
			VisP p = ParticlesVis::NewP();
			p->LoadFromJSON(vv);
			AddOsprayGeometryVis(p);
		}
		else if (t == "Triangles")
		{
			VisP p = TrianglesVis::NewP();
			p->LoadFromJSON(vv);
			AddOsprayGeometryVis(p);
		}
		else
		{
			cerr << "ERROR: unknown type: " << t << " in json Visualization element" << endl;
			exit(1);
		}
	}
}

int 
Visualization::serialSize()
{
	return KeyedObject::serialSize() + (sizeof(int) + annotation.length() + 1) 
		+ sizeof(int) + osprayGeometries.size()*sizeof(Key)
		+ sizeof(int) + mappedGeometries.size()*sizeof(Key)
		+ sizeof(int) + volumes.size()*sizeof(Key) 
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

	*(int *)p = osprayGeometries.size();
	p += sizeof(int);

	for (auto v : osprayGeometries)
	{
		*(Key *)p = v->getkey();
		p += sizeof(Key);
	}

	*(int *)p = mappedGeometries.size();
	p += sizeof(int);

	for (auto v : mappedGeometries)
	{
		*(Key *)p = v->getkey();
		p += sizeof(Key);
	}

	*(int *)p = volumes.size();
	p += sizeof(int);

	for (auto v : volumes)
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
		AddOsprayGeometryVis(Vis::GetByKey(k));
	}

	n = *(int *)p;
	p += sizeof(int);

	for (int i = 0; i < n; i++)
	{
		Key k = *(Key *)p;
		p += sizeof(Key);
		AddMappedGeometryVis(Vis::GetByKey(k));
	}

	n = *(int *)p;
	p += sizeof(int);

	for (int i = 0; i < n; i++)
	{
		Key k = *(Key *)p;
		p += sizeof(Key);
		AddVolumeVis(Vis::GetByKey(k));
	}

	return p;
}

void 
Visualization::destroy_ispc()
{
  if (ispc)
  {
    //cerr << "Visualization destroying " << this << " ispc: " << ispc << endl;
    ispc::Visualization_destroy(ispc);
  }
}

} // namespace gxy
