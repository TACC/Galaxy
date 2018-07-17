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

#pragma once

#include <memory>
#include <string>

#include <vtkSmartPointer.h>
#include <vtkClientSocket.h>
#include <vtkImageData.h>

#include "Box.h"
#include "KeyedObject.h"

namespace gxy
{

KEYED_OBJECT_POINTER(KeyedDataObject)

class KeyedDataObject : public KeyedObject
{
	KEYED_OBJECT(KeyedDataObject)

	friend class ImportMsg;
	friend class AttachMsg;
	friend class LoadTimestepMsg;

public:
	virtual ~KeyedDataObject();
	virtual void initialize();

	virtual bool local_commit(MPI_Comm);

	Box *get_global_box() { return &global_box; }
	Box *get_local_box() { return &local_box; }

	int get_neighbor(int i) { return neighbors[i]; }
  bool has_neighbor(unsigned int face) { return neighbors[face] >= 0; }

	bool is_time_varying() { return time_varying; }
	void set_time_varying(bool t = true) { time_varying = t; }

	bool is_attached() { return attached; }
	void set_attached(bool t = true) { attached = t; }

	virtual void Import(std::string);
	virtual void Import(std::string, void *args, int argsSize);

  virtual bool Attach(std::string);
  virtual bool Attach(std::string, void *args, int argsSize);

	virtual void LoadTimestep();
	virtual bool WaitForTimestep();

	void CopyPartitioning(KeyedDataObjectP o);

protected:
	vtkClientSocket *skt;
	std::string filename;
	
	bool time_varying, attached;

  virtual void local_import(char *, MPI_Comm c);
  virtual bool local_load_timestep(MPI_Comm c);

	Box global_box, local_box;
	int neighbors[6];

  class ImportMsg : public Work
  {
  public:
    ImportMsg(Key k, std::string vname, void *args, int argsSize) : ImportMsg(sizeof(Key) + vname.length() + 1 + argsSize)
    {
      unsigned char *p = contents->get();

      *(Key *)p = k;
      p += sizeof(Key);

      memcpy(p, vname.c_str(), vname.length()+1);
      p += vname.length()+1;

      if (argsSize)
        memcpy(p, args, argsSize);
    }

    WORK_CLASS(ImportMsg, true);

  public:
    bool CollectiveAction(MPI_Comm c, bool isRoot)
		{
			char *p = (char *)contents->get();

			Key k = *(Key *)p;
			p += sizeof(Key);

			KeyedDataObjectP o = KeyedDataObject::GetByKey(k);

			o->local_import(p, c);

			return false;
		}
  };

  class AttachMsg : public Work
  {
  public:
    AttachMsg(Key k, std::string vname) : AttachMsg(sizeof(Key) + vname.length() + 1)
    {
      *(Key *)contents->get() = k;
      memcpy(((char *)contents->get()) + sizeof(Key), vname.c_str(), vname.length()+1);
    }

    WORK_CLASS(AttachMsg, true);

  public:
    bool CollectiveAction(MPI_Comm c, bool isRoot);
  };

  class LoadTimestepMsg : public Work
  {
  public:
    LoadTimestepMsg(Key k) : LoadTimestepMsg(sizeof(Key))
    {
      *(Key *)contents->get() = k;
    }

    WORK_CLASS(LoadTimestepMsg, true);

  public:
    bool CollectiveAction(MPI_Comm c, bool isRoot)
		{
			Key k = *(Key *)contents->get();
			KeyedDataObjectP o = KeyedDataObject::GetByKey(k);
			o->local_load_timestep(c);
			return false;
		}
  };
};

} // namespace gxy
