#include <iostream>
#include <sstream>
#include <vector>

#include "Application.h"
#include "Visualization.h"
#include "OSPUtil.h"
#include "Visualization_ispc.h"

#define LOGGING 0

using namespace rapidjson;
// #include "../rapidjson/document.h"
// #include "../rapidjson/prettywriter.h"
// #include "../rapidjson/stringbuffer.h"


using namespace std;

namespace pvol
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
		if (! kdop) std::cerr << "NULL KDOP\n";	\
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
        APP_PRINT(<< "Datasets: partitioning mismatch between participants");	  \
        exit(0);																															  \
      }																																				  \
      for (int i = 0; i < 6; i++)																							  \
      {																																				  \
        if (neighbors[i] != kdop->get_neighbor(i))														  \
        {																																			  \
          APP_PRINT(<< "Datasets: partitioning mismatch between participants");	\
          exit(0);																															\
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
    ospAddGeometry(ospModel, (OSPGeometry)ospg->GetTheData()->GetOSP());
  
  ospCommit(ospModel);
   
  //std::cerr << "volumes: " << volumes.size() <<  " " <<  volumes.data() << "\n";
  //std::cerr << "mappedGeometries: " << mappedGeometries.size() <<  " " <<  mappedGeometries.data() << "\n";
  
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

  ops = v["operators"];
	for (int i = 0; i < ops.Size(); i++)
	{
		Value& vv = ops[i];

		if (! vv.HasMember("type"))
		{
			std::cerr << "Visualization element with no type\n";
			exit(1);
		}

		string t = string(vv["type"].GetString());

		VisP vp;
		if (t == "Volume")
		{
			vp = VolumeVis::NewP();
			vp->LoadFromJSON(vv);
			AddVolumeVis(vp);
		}
		else if (t == "Particles")
		{
			VisP p = Vis::NewP();
			p->LoadFromJSON(vv);
			AddOsprayGeometryVis(p);
		}
		else
		{
			std::cerr << "unknown vis type: " << t << "\n";
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
		+ sizeof(int) + volumes.size()*sizeof(Key);
}

unsigned char *
Visualization::serialize(unsigned char *p)
{
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
    //std::cerr << "Visualization destroying " << this << " ispc: " << ispc << "\n";
    ispc::Visualization_destroy(ispc);
  }
}

}
