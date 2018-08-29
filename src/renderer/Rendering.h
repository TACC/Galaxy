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

KEYED_OBJECT_POINTER(Rendering)

class Ray;

class Rendering : public KeyedObject
{
	KEYED_OBJECT(Rendering);
	virtual ~Rendering();								// to get rid of the FB

public:
	static void RegisterMessages()
	{
	}
	
	virtual void initialize();

  virtual bool local_commit(MPI_Comm);
  virtual void local_reset();

	Key GetTheRenderingSetKey();
	RenderingSetP GetTheRenderingSet();
	void SetTheRenderingSet(Key rsk);
	void SetTheRenderingSet(RenderingSetP rs);

	Key GetTheCameraKey();
	CameraP GetTheCamera();
	void SetTheCamera(Key c);
	void SetTheCamera(CameraP c);

	Key GetTheDatasetsKey();
	DatasetsP GetTheDatasets();
	void SetTheDatasets(Key c);
	void SetTheDatasets(DatasetsP c);

	Key GetTheVisualizationKey();
	VisualizationP GetTheVisualization();
	void SetTheVisualization(Key v);
	void SetTheVisualization(VisualizationP v);

	int GetTheOwner() { return owner; }
	void SetTheOwner(int o) { owner = o; }

	int  GetTheWidth()  { return width; }
	int  GetTheHeight() { return height; }
	void GetTheSize(int& w, int &h) { w = width; h = height; }

	void SetTheSize(int w, int h);

	void AllocateFrameBuffer()
	{
		if (framebuffer) delete[] framebuffer;
		framebuffer = new float[width * height * 4];
#ifndef GXY_WRITE_IMAGES
		if (kbuffer) delete[] kbuffer;
#endif
	}

	virtual void AddLocalPixels(Pixel *, int, int, int sender = -1);	// Add to local FB from received send buffer

	bool IsLocal();
	void SaveImage(std::string, int);

	virtual int serialSize();
	virtual unsigned char *serialize(unsigned char *);
	virtual unsigned char *deserialize(unsigned char *);

	float *GetPixels() { return framebuffer; }
	Lighting *GetLighting() { return &lights; }

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
