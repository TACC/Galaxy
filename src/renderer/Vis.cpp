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

#include "Vis.h"

#include "Application.h"
#include "KeyedDataObject.h"
#include "MappedVis.h"
#include "OsprayUtil.h"
#include "ParticlesVis.h"
#include "PathLinesVis.h"
#include "TrianglesVis.h"
#include "Vis_ispc.h"
#include "VolumeVis.h"

#include "rapidjson/document.h"

using namespace rapidjson;
using namespace std;

namespace gxy
{

KEYED_OBJECT_CLASS_TYPE(Vis)

void
Vis::Register()
{
	RegisterClass();
	MappedVis::Register();
	VolumeVis::Register();
	ParticlesVis::Register();
	PathLinesVis::Register();
	TrianglesVis::Register();
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
  ispc::Vis_initialize(GetIspc());
}

void 
Vis::destroy_ispc()
{
  ispc::Vis_destroy(GetIspc());
}

bool 
Vis::Commit(DatasetsP datasets)
{
	datakey = datasets->FindKey(name);
	if (datakey == -1)
	{
		std::cerr << "ERROR: Unable to find data using name: " << name << endl;
		set_error(1);
    return false;
	}
	return KeyedObject::Commit();
}

bool
Vis::LoadFromJSON(Value& v)
{
  if (v.HasMember("dataset"))
  {
		name = string(v["dataset"].GetString());
    return true;
  }
	else
	{
		cerr << "ERROR: json Vis block has no dataset" << endl;
		return false;
	}
}

void
Vis::SetTheOsprayDataObject(OsprayObjectP o)
{
  odata = o;
	ispc::Vis_set_data(GetIspc(), o->GetOSP_IE());
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

  data = KeyedDataObject::GetByKey(datakey);

	return false;
}

} // namespace gxy
