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
#include "IspcObject.h"
#include "Lighting.h"
#include "Rays.h"
#include "Visualization.h"
#include "Partitioning.h"

namespace gxy
{
OBJECT_POINTER_TYPES(TraceRays)

//! controls the ray tracing loop within Galaxy
/*! \sa KeyedObject, IspcObject
 * \ingroup render
 */
class TraceRays : public IspcObject
{
public:
  TraceRays(float epsilon, PartitioningP p);
  ~TraceRays(); //!< default destructor

  //! trace a given RayList against the given Visualization using the given Lighting
  /*! \returns a RayList pointer to rays spawned during this trace
   * \param lights a pointer to the Lighting object to use during this trace
   * \param visualization a pointer to the Visualization to trace
   * \param raysIn a pointer to the RayList of rays to trace
   */
  RayList *Trace(Lighting* lights, VisualizationP visualization, RayList * raysIn);

protected:
  virtual void allocate_ispc();
  virtual void initialize_ispc();
  virtual void destroy_ispc();

  float epsilon;
  PartitioningP partitioning;
};

} // namespace gxy
