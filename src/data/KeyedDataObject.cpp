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
#include "KeyedDataObject.h"
#include "Datasets.h"
#include "Geometry.h"
#include "Volume.h"
#include "AmrVolume.h"

using namespace std;

namespace gxy
{

WORK_CLASS_TYPE(KeyedDataObject::ImportMsg);
WORK_CLASS_TYPE(KeyedDataObject::CopyMsg);

void
KeyedDataObject::Register()
{
  RegisterClass();

  ImportMsg::Register();
  CopyMsg::Register();

  Datasets::Register();
  Geometry::Register();
  Volume::Register();
  AmrVolume::Register();
}

KEYED_OBJECT_CLASS_TYPE(KeyedDataObject)

KeyedDataObject::~KeyedDataObject()
{
  if (skt)          
  {
    skt->CloseSocket();
    skt->Delete();
  }
}

KeyedDataObjectP
KeyedDataObject::Copy()
{
  KeyedDataObjectP dst = Cast(GetTheKeyedObjectFactory()->NewP(GetClassType()));
  CopyMsg msg(getkey(), dst->getkey());
  msg.Broadcast(true, true);
  return dst;
}

bool
KeyedDataObject::local_copy(KeyedDataObjectP src)
{
  CopyPartitioning(src);
  modified = false;
  skt = NULL;
  filename = "";
  time_varying = false;
  attached = false;
  return true;
}

OsprayObjectP
KeyedDataObject::CreateTheOSPRayEquivalent(KeyedDataObjectP kdop)
{
  return NULL;
}

void 
KeyedDataObject::CopyPartitioning(KeyedDataObject* o)
{
	local_box = *o->get_local_box();
	global_box = *o->get_global_box();
	for (int i = 0; i < 6; i++)
			neighbors[i] = o->get_neighbor(i);
}

void 
KeyedDataObject::initialize()
{
	super::initialize();
  time_varying = false;
  attached = false;
  skt = NULL;
}

bool
KeyedDataObject::local_commit(MPI_Comm c)
{
  setModified(true);
  return false;
}

bool
KeyedDataObject::Import(string filename) { return Import(filename, NULL, 0); }

bool
KeyedDataObject::Import(string filename, void *args, int argsSize)
{
  ImportMsg msg(getkey(), filename, args, argsSize);
  msg.Broadcast(true, true);
  return get_error() == 0;
}

bool
KeyedDataObject::local_import(char *s, MPI_Comm c)
{
  std::cerr << "ERROR: generic KeyedDataObject::local_import called?" << std::endl;
  return false;
}

void
KeyedDataObject::set_neighbors(int *n)
{
  memcpy(neighbors, n, 6*sizeof(int));
}


} // namespace gxy
