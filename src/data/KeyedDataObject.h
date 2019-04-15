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

#pragma once

/*! \file KeyedDataObject.h 
 * \brief base class for registered (i.e. tracked) data objects within Galaxy
 * \ingroup data
 */

#include <memory>
#include <string>

#include <vtkSmartPointer.h>
#include <vtkClientSocket.h>
#include <vtkImageData.h>

#include "Box.h"
#include "KeyedObject.h"

namespace gxy
{

OBJECT_POINTER_TYPES(KeyedDataObject)

//! base class for registered (i.e. tracked) data objects within Galaxy
/*! Galaxy maintains a global registry of objects in order to route data to the appropriate process. 
 * In order to be tracked in the registry, the object should derive from KeyedDataObject and call the
 * OBJECT_POINTER_TYPES macro in its header file (outside of the class definition) and the 
 * KEYED_OBJECT or KEYED_OBJECT_SUBCLASS macro in its class definition.
 *
 * \ingroup data
 * \sa KeyedObject, KeyedObjectFactory, Work
 */
class KeyedDataObject : public KeyedObject
{
	KEYED_OBJECT_SUBCLASS(KeyedDataObject, KeyedObject)

	friend class ImportMsg;
	friend class AttachMsg;
	friend class LoadTimestepMsg;

public:
	virtual ~KeyedDataObject(); //!< destructor
	virtual void initialize(); //!< initialize this KeyedDataObject

  //! commit this object to the local registry
	virtual bool local_commit(MPI_Comm);

  //! returns a pointer to a Box that represents the global data extent across all processes
	Box *get_global_box() { return &global_box; }
  //! returns a pointer to a Box that represents the local data extent at this process
	Box *get_local_box() { return &local_box; }

  //! return the data key that is the `i^th` neighbor of this proces
  /*! This method uses the Box face orientation indices for neighbor indexing
   *          - yz-face neighbors - `0` for the lower (left) `x`, `1` for the higher (right) `x`
   *          - xz-face neighbors - `2` for the lower (left) `y`, `3` for the higher (right) `y`
   *          - xy-face neighbors - `4` for the lower (left) `z`, `5` for the higher (right) `z`
   */
	int get_neighbor(int i) { return neighbors[i]; }
  //! return true if the requested neighbor exists
  /*! This method uses the Box face orientation indices for neighbor indexing
   *          - yz-face neighbors - `0` for the lower (left) `x`, `1` for the higher (right) `x`
   *          - xz-face neighbors - `2` for the lower (left) `y`, `3` for the higher (right) `y`
   *          - xy-face neighbors - `4` for the lower (left) `z`, `5` for the higher (right) `z`
   */
  bool has_neighbor(unsigned int face) { return neighbors[face] >= 0; }

  //! is this KeyedDataObject time varying?
	bool is_time_varying() { return time_varying; }
  //! set whether this KeyedDataObject is time varying
	void set_time_varying(bool t = true) { time_varying = t; }

  //! is this KeyedDataObject attached to a dynamic data source (e.g. a running simulation in an in situ context)?
	bool is_attached() { return attached; }
  //! set whether this KeyedDataObject is attached to a dynamic data source 
	void set_attached(bool t = true) { attached = t; }

  //! broadcast an ImportMsg to all Galaxy processes to import the given data file
	virtual bool Import(std::string);

  //! broadcast an ImportMsg to all Galaxy processes to import the given data file using the given arguments
	virtual bool Import(std::string, void *args, int argsSize);

  //! broadcat an AttachMsg to all Galaxy processes to attach to a data source at the given address and port
  virtual bool Attach(std::string);
  //! broadcat an AttachMsg to all Galaxy processes to attach to a data source at the given address and port using the given arguments
  virtual bool Attach(std::string, void *args, int argsSize);

  //! broadcast a LoadTimestepMsg to all Galaxy processes for all attached data sources (e.g. a running simulation for in situ analysis)
	virtual bool LoadTimestep(); 
  //! wait for receipt of next timestep for all attached data sources (e.g. a running simulation for in situ analysis)
	virtual bool WaitForTimestep();

  //! copy the data partitioning of the given KeyedDataObject
	void CopyPartitioning(KeyedDataObjectP o);

  float local_min, local_max;
  float global_min, global_max;

  void set_global_minmax(float min, float max)   { global_min = min; global_max = max; }
  void set_local_minmax(float min, float max)   { local_min = min; local_max = max; }

  void get_global_minmax(float& min, float& max)   { min = global_min; max = global_max; }
  void get_local_minmax(float& min, float& max)   { min = local_min; max = local_max; }

protected:
	vtkClientSocket *skt;
	std::string filename;
	
	bool time_varying, attached;

  virtual bool local_import(char *, MPI_Comm c);
  virtual bool local_load_timestep(MPI_Comm c);

	Box global_box, local_box;
	int neighbors[6];

  //! tell a Galaxy processes to import a given data file
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

			if (!o->local_import(p, c))
        o->set_error(1);

			return false;
		}
  };

  //! tell a Galaxy process to attach to a dynamic data source
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

  //! tell a Galaxy process to load a timestep from a dynamic data source
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
      int ge, le = o->get_error();
      MPI_Allreduce(&le, &ge, 1, MPI_INT, MPI_MAX, c);
      o->set_error(ge);
			return false;
		}
  };
};

} // namespace gxy

