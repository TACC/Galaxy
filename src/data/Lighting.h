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

/*! \file Lighting.h 
 * \brief represents a light source within Galaxy
 * \ingroup data
 */

#include <iostream>
#include <vector>

#include "dtypes.h"
#include "rapidjson/document.h"
#include "ISPCObject.h"

namespace gxy
{

//! represents a light source within Galaxy
/*! \ingroup data 
 * \sa ISPCObject
 */  
class Lighting : public ISPCObject
{
public:
   Lighting(); //!< default constructor
  ~Lighting(); //!< default destructor

  //! returns the size in bytes of the serialization of this Lighting object
  virtual int SerialSize();
  //! serialize this Lighting object to a given byte array
  virtual unsigned char *Serialize(unsigned char *);
  //! deserialize a Lighting object from a given byte array into this object
  virtual unsigned char *Deserialize(unsigned char *);

  //! load a Lighting object from a given Galaxy JSON Value object into this object
  void LoadStateFromValue(rapidjson::Value&);
  //! save this Lighting object as a JSON Value object to a given Galaxy JSON document
  void SaveStateToValue(rapidjson::Value&, rapidjson::Document&);

  //! set a number of lights with a given luminance and type
  /*! \param n number of lights
   * \param l an `n * RGB` value array of luminance values for each light
   * \param t an `n` value array of OSPRay light type values for each light
   */
  void SetLights(int n, float* l, int* t);
  //! get the number of defined lights, with luminance values and types of each
  /*! \param n number of lights
   * \param l an `n * RGB` value array of luminance values for each light
   * \param t an `n` value array of OSPRay light type values for each light
   */
  void GetLights(int& n, float*& l, int*& t);

  //! set the ambient and diffuse lighting values for this Lighting object
  void SetK(float ka, float kd);
  //! get the ambient and diffuse lighting values for this Lighting object
  void GetK(float& ka, float& kd);

  //! set the number of ambient occlusion rays to be cast within the given radius 
  /*! \param n the number of ambient occlusion (AO) rays to be cast
   * \param r the distance from their origin each ray should travel. 
   *          If a ray reaches its maximum distance without hitting anything, the origin is considered unoccluded by that ray
   */
  void SetAO(int n, float r);
  //! get the number of ambient occlusion rays to be cast within the given radius 
  /*! \param n the number of ambient occlusion (AO) rays to be cast
   * \param r the distance from their origin each ray should travel. 
   *          If a ray reaches its maximum distance without hitting anything, the origin is considered unoccluded by that ray
   */
  void GetAO(int& n, float& r);

  //! set whether Galaxy should render shadows
  void SetShadowFlag(bool b);
  //! get whether Galaxy will render shadows
  void GetShadowFlag(bool& b);

  //! has this Lighting object been initialized?
	bool isSet() { return set; }

protected:
	int set;
  virtual void allocate_ispc();
  virtual void initialize_ispc();
};

} // namespace gxy

