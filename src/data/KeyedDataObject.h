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

#pragma once

/*! \file KeyedDataObject.h 
 * \brief base class for registered (i.e. tracked) data objects within Galaxy
 * \ingroup data
 */

#include <memory>
#include <string>
#include <pthread.h>

#include <vtkSmartPointer.h>
#include <vtkClientSocket.h>
#include <vtkImageData.h>

#include "Box.h"
#include "KeyedObject.h"
#include "ObjectFactory.h"

namespace gxy
{

KEYED_OBJECT_POINTER_TYPES(KeyedDataObject)

//! base class for registered (i.e. tracked) data objects within Galaxy
/*! Galaxy maintains a global registry of objects in order to route data to the appropriate process. 
 * In order to be tracked in the registry, the object should derive from KeyedDataObject and call the
 * OBJECT_POINTER_TYPES macro in its header file (outside of the class definition) and the 
 * KEYED_OBJECT or KEYED_OBJECT_SUBCLASS macro in its class definition.
 *
 * \ingroup data
 * \sa KeyedObject, ObjectFactory, Work
 */
class KeyedDataObject : public KeyedObject
{
  KEYED_OBJECT_SUBCLASS(KeyedDataObject, KeyedObject)

  friend class ImportMsg;

public:
  virtual ~KeyedDataObject(); //!< destructor
  virtual void initialize(); //!< initialize this KeyedDataObject

  virtual KeyedDataObjectDPtr Copy();
  virtual bool local_copy(KeyedDataObjectDPtr src);

  //! commit this object to the local registry
  virtual bool local_commit(MPI_Comm);

  //! returns a pointer to a Box that represents the global data extent across all processes
  Box *get_global_box() { return &global_box; }
  //! returns a pointer to a Box that represents the local data extent at this process
  Box *get_local_box() { return &local_box; }

  //! install neighbors array
  void set_neighbors(int*);

  //! return the data key that is the `i^th` neighbor of this process
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

  //! copy the data partitioning of the given KeyedDataObject
  void CopyPartitioning(KeyedDataObjectDPtr o) { CopyPartitioning(o.get()); };

  float local_min, local_max;
  float global_min, global_max;

  void set_global_minmax(float min, float max)   { global_min = min; global_max = max; }
  void set_local_minmax(float min, float max)   { local_min = min; local_max = max; }

  void get_global_minmax(float& min, float& max)   { min = global_min; max = global_max; }
  void get_local_minmax(float& min, float& max)   { min = local_min; max = local_max; }

  virtual void SetTheDeviceEquivalent(GalaxyObjectPtr de) { device_equivalent = de; }
  GalaxyObjectPtr GetTheDeviceEquivalent() { return device_equivalent; }

  void set_boxes(Box l, Box g) {local_box = l; global_box = g;};

  void setModified(bool m) { modified = m; }
  bool hasBeenModified() { return modified; }

protected:
  void CopyPartitioning(KeyedDataObject* o);
  GalaxyObjectPtr device_equivalent;

  bool modified;
  vtkClientSocket *skt;
  std::string filename;

  bool time_varying, attached;

  virtual bool local_import(char *, MPI_Comm c);

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

      KeyedDataObjectDPtr o = KeyedDataObject::GetByKey(k);

      if (!o->local_import(p, c))
        o->set_error(1);

      return false;
    }
  };

  //! tell a Galaxy processes to copy the guts of src to dst

  class CopyMsg : public Work
  {
  public:
    CopyMsg(Key src, Key dst) : CopyMsg(2*sizeof(Key))
    {
      unsigned char *p = contents->get();

      *(Key *)p = src;
      p += sizeof(Key);

      *(Key *)p = dst;
      p += sizeof(Key);
    }

    WORK_CLASS(CopyMsg, true);

  public:
    bool CollectiveAction(MPI_Comm c, bool isRoot)
    {
      char *p = (char *)contents->get();

      Key k = *(Key *)p;
      p += sizeof(Key);
      KeyedDataObjectDPtr src = KeyedDataObject::GetByKey(k);

      k = *(Key *)p;
      p += sizeof(Key);
      KeyedDataObjectDPtr dst = KeyedDataObject::GetByKey(k);


      if (!dst->local_copy(src))
        dst->set_error(1);

      return false;
    }
  };
};

} // namespace gxy

