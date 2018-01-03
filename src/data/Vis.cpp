#include <iostream>
#include <math.h>
#include <stdlib.h>
#include <string>
#include <sstream>
#include <fstream>
#include "Application.h"
#include "Vis.h"
#include "VolumeVis.h"
#include "MappedVis.h"

#include "OSPUtil.h"

#include "Vis_ispc.h"

using namespace std;

KEYED_OBJECT_TYPE(Vis)

void
Vis::Register()
{
	RegisterClass();
	MappedVis::Register();
	VolumeVis::Register();
}

Vis::~Vis()
{
}

void 
Vis::allocate_ispc()
{
  ispc = ispc::Vis_allocate();
}

void 
Vis::initialize_ispc()
{
  ispc::Vis_initialize(GetISPC());
}

void 
Vis::destroy_ispc()
{
  ispc::Vis_destroy(GetISPC());
}

void 
Vis::Commit(DatasetsP datasets)
{
	datakey = datasets->FindKey(name);
	if (datakey == -1)
	{
		std::cerr << "Unable to find data using name: " << name << "\n";
		exit(1);
	}
	KeyedObject::Commit();
}

void 
Vis::LoadFromJSON(Value& v)
{
  if (v.HasMember("dataset"))
		name = string(v["dataset"].GetString());
	else
	{
		cerr << "Vis has no dataset\n";
		exit(0);
	}
}

void 
Vis::SaveToJSON(Value& v, Document& doc)
{
	v.AddMember("dataset", Value().SetString(name.c_str(), doc.GetAllocator()), doc.GetAllocator());
}

int
Vis::serialSize() 
{
	return KeyedObject::serialSize() + sizeof(Key);
}

unsigned char *
Vis::deserialize(unsigned char *ptr) 
{
	ptr = KeyedObject::deserialize(ptr);

	datakey = *(Key *)ptr;
	ptr += sizeof(datakey);

	return ptr;
}

unsigned char *
Vis::serialize(unsigned char *ptr)
{
	ptr = KeyedObject::serialize(ptr);

	*(Key *)ptr = datakey;
	ptr += sizeof(Key);

	return ptr;
}

bool 
Vis::local_commit(MPI_Comm c)
{
	if (! ispc)
  {
    allocate_ispc();
    initialize_ispc();
  }

	super::local_commit(c);

	data = OSPRayObject::Cast(KeyedDataObject::GetByKey(datakey));

  void *dispc = data->GetOSP_IE();
  ispc::Vis_set_data(GetISPC(), dispc);

	return false;
}
