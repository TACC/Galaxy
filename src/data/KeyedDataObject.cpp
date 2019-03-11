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
WORK_CLASS_TYPE(KeyedDataObject::AttachMsg);
WORK_CLASS_TYPE(KeyedDataObject::LoadTimestepMsg);

void
KeyedDataObject::Register()
{
  RegisterClass();

  ImportMsg::Register();
  AttachMsg::Register();
  LoadTimestepMsg::Register();

  Datasets::Register();
  Geometry::Register();
  Volume::Register();
  AmrVolume::Register();
}

KEYED_OBJECT_CLASS_TYPE(KeyedDataObject)

KeyedDataObject::~KeyedDataObject()
{
  //std::cerr << "~KeyedDataObject: " << this << " skt " << skt << std::endl;
  if (skt)          
  {
    skt->CloseSocket();
    skt->Delete();
  }
}

void 
KeyedDataObject::CopyPartitioning(KeyedDataObjectP o)
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
  return false;
}

bool
KeyedDataObject::Attach(string layout) { return Attach(layout, NULL, 0); }

bool
KeyedDataObject::Attach(string layout, void *args, int argsSize)
{
  AttachMsg msg(sizeof(Key) + layout.length() + 1 + argsSize);
  char *p = (char *)msg.get();
  *(Key *)p = getkey();
  p = p + sizeof(Key);
  memcpy(p, layout.c_str(), layout.length());
  p = p + layout.length();
  *p++ = 0;

  if (argsSize > 0)
    memcpy(p, args, argsSize);

  msg.Broadcast(true, true);
  return skt != NULL;
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
KeyedDataObject::WaitForTimestep()
{
  if (! skt)
    return false;

  int flag = -1;
  if ((skt->Receive((void *)&flag, sizeof(flag)) != sizeof(flag)) || (flag < 1))
  {
    skt->CloseSocket();
    skt->Delete();
    skt = NULL;
    return false;
  }
  else
    return true;
}

bool
KeyedDataObject::LoadTimestep()
{
  LoadTimestepMsg msg(getkey());
  msg.Broadcast(true, true);
  return get_error() != 0;
}

bool
KeyedDataObject::local_import(char *s, MPI_Comm c)
{
  std::cerr << "ERROR: generic KeyedDataObject::local_import called?" << std::endl;
  return false;
}

bool
KeyedDataObject::local_load_timestep(MPI_Comm c)
{
  std::cerr << "ERROR: generic KeyedDataObject::local_load_timestep called?" << std::endl;
  set_error(1);
  return false;
}

bool
KeyedDataObject::AttachMsg::CollectiveAction(MPI_Comm c, bool isRoot)
{
  char *p = (char *)contents->get();
  Key k = *(Key *)p;
  p += sizeof(Key);

  KeyedDataObjectP o = KeyedDataObject::GetByKey(k);

  int rank = GetTheApplication()->GetRank();

  string server;
  int port;

  ifstream ifs(p);
  for (int i = 0; i <= rank; i++)
  {
    if (ifs.eof()) {cerr << "ERROR: could not get attachment point for CollectiveAction message" << endl; exit(1); }
    ifs >> server >> port;
  }

  o->skt = vtkClientSocket::New();
  if (o->skt->ConnectToServer(server.c_str(), port))
  {
    o->skt->Delete();
    o->skt = NULL;
  }

  return false;
}

} // namespace gxy
