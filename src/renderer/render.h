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

/*! \file render.h
 *  \brief A convenience header to include all Galaxy render headers.
 */

/*! \defgroup render Render 
 * \brief all classes for Galaxy's supported render types and operations
 */

#include "Camera.h"
#include "hits.h"
#include "ImageWriter.h"
#include "IspcObject.h"
#include "Lighting.h"
#include "MappedVis.h"
#include "mypng.h"
#include "ParticlesVis.h"
#include "Pixel.h"
#include "RayFlags.h"
#include "RayQManager.h"
#include "Rays.h"
#include "Renderer.h"
#include "Rendering.h"
#include "RenderingEvents.h"
#include "RenderingSet.h"
#include "TraceRays.h"
#include "TrianglesVis.h"

