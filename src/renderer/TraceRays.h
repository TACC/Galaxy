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

/*! \file TraceRays.h 
 * \brief controls the ray tracing loop within Galaxy
 * \ingroup render
 */

#include <memory>
#include <string.h>
#include <string>
#include <vector>

#include "dtypes.h"
#include "ISPCObject.h"
#include "Lighting.h"
#include "Rays.h"
#include "Visualization.h"

namespace gxy
{
OBJECT_POINTER_TYPES(TraceRays)

//! controls the ray tracing loop within Galaxy
/*! \sa KeyedObject, ISPCObject
 * \ingroup render
 */
class TraceRays : public ISPCObject
{
public:
  TraceRays(); //!< default constructor
  ~TraceRays(); //!< default destructor

  //! populate state into this TraceRays object from a Galaxy JSON object
  virtual bool LoadStateFromValue(rapidjson::Value&);
  //!< save the state of this TraceRays object to a Galaxy JSON object
  virtual void SaveStateToValue(rapidjson::Value&, rapidjson::Document&); 

  void SetEpsilon(float e); //!< set the epsilon ray offset value for use by this TraceRays object
  float GetEpsilon(); //!< return the current epsilon ray offset value used by this TraceRays object

  //! trace a given RayList against the given Visualization using the given Lighting
  /*! \returns a RayList pointer to rays spawned during this trace
   * \param lights a pointer to the Lighting object to use during this trace
   * \param visualization a pointer to the Visualization to trace
   * \param raysIn a pointer to the RayList of rays to trace
   */
  RayList *Trace(Lighting* lights, VisualizationP visualization, RayList * raysIn);

  //! return the size in bytes of this TraceRays object
  virtual int SerialSize(); 
  //! serialize this TraceRays object to the given byte stream
  virtual unsigned char *Serialize(unsigned char *); 
  //! deserialize a TraceRays object from the given byte stream into this object
  virtual unsigned char *Deserialize(unsigned char *); 

protected:

  virtual void allocate_ispc();
  virtual void initialize_ispc();

};

} // namespace gxy
