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

/*! \file Rendering.h 
 * \brief represents an image of a Visualization rendered using a certain Camera 
 * \ingroup render
 */

#include <memory>
#include <string>
#include <vector>

#include "Application.h"
#include "Camera.h"
#include "Datasets.h"
#include "KeyedObject.h"
#include "KeyedObject.h"
#include "Lighting.h"
#include "Pixel.h"
#include "RenderingSet.h"
#include "Visualization.h"
#include "Work.h"

namespace gxy
{

OBJECT_POINTER_TYPES(Rendering)

class Ray;

//! represents an image of a Visualization rendered using a certain Camera 
/*! \sa KeyedObject
 * \ingroup render
 */
class Rendering : public KeyedObject
{
	KEYED_OBJECT(Rendering);
	//! default destructor
	virtual ~Rendering();								// to get rid of the FB

public:
	//! register message types with the Galaxy system (currently noop)
	static void RegisterMessages()
	{
	}
	
	virtual void initialize(); //!< initialize this Rendering object

	//! (re-)allocate the local framebuffer and image buffers for this Rendering, if owned by this process
  virtual bool local_commit(MPI_Comm);
  //! (re-)initialize the local framebuffer and image buffers to zero, if owned by this process
  virtual void local_reset();

	CameraP GetTheCamera(); //!< returns a pointer to the Camera used for this Rendering
	void SetTheCamera(CameraP c); //!< set the Camera to use for this Rendering

	DatasetsP GetTheDatasets(); //!< returns a pointer to the Datasets used in this Rendering
	void SetTheDatasets(DatasetsP c); //!< set the Datasets to use for this Rendering

	VisualizationP GetTheVisualization(); //!< returns a pointer to the Visualization used in this Rendering
	void SetTheVisualization(VisualizationP v); //!< set the Visualization to use for this Rendering

	int GetTheOwner() { return owner; } //!< returns the rank of the process that owns the framebuffer for this Rendering
	void SetTheOwner(int o) { owner = o; } //!< set the rank of the process that owns this Rendering

	int  GetTheWidth()  { return width; } //!< return the width of the framebuffer for this Rendering
	int  GetTheHeight() { return height; } //!< return the height of the framebuffer for this Rendering
	//! return the width and height of the framebuffer for this Rendering
	void GetTheSize(int& w, int &h) { w = width; h = height; }
	//! set the width and height of the framebuffer for this Rendering
	void SetTheSize(int w, int h) { width = w; height = h; }

	//! allocate the framebuffer for this Rendering according to the current width and height
	void AllocateFrameBuffer()
	{
		if (framebuffer) delete[] framebuffer;
		framebuffer = new float[width * height * 4];
#ifndef GXY_WRITE_IMAGES
		if (kbuffer) delete[] kbuffer;
#endif
	}

	//! add the given Pixel contributions for the specified frame to the local framebuffer
	/*! \param p an array of Pixel objects to add 
	 * \param n the number of Pixel objects in the array
	 * \param f the frame number to which these Pixels belong
	 * \param sender the rank of the process from which these Pixels were received
	 */
	virtual void AddLocalPixels(Pixel *p, int n, int f, int sender = -1);	// Add to local FB from received send buffer

	bool IsLocal(); //!< returns true if the calling process owns this Rendering (i.e. if the process rank matches the owner tag)
	void SaveImage(std::string, int); //!< save the current framebuffer to a PNG file using the given filename base and index increment

	virtual int serialSize(); //!< returns the size in bytes for the serialization of this Rendering
	virtual unsigned char *serialize(unsigned char *); //!< serialize this Rendering to the given byte array
	virtual unsigned char *deserialize(unsigned char *); //!< deserialize a Rendering from the given byte array into this Rendering

	//! return a pointer to the framebuffer for this Rendering
	float *GetPixels() { return framebuffer; }
	//! return a pointer to the Lighting singleton for this rendering
	Lighting *GetLighting() { return &lights; }
	//! set lights for this Rendering using the given Renderer
	void resolve_lights(RendererP);
	
protected:
	Lighting lights;
	int frame;

	VisualizationP visualization;
	CameraP    		 camera;
	DatasetsP  		 datasets;

	int owner;
	int accumulation_knt;

	float *framebuffer;
#ifndef GXY_WRITE_IMAGES
  int *kbuffer;
#endif

	int width, height;
};

} // namespace gxy
