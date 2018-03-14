#pragma once

#include <string.h>
#include <vector>
#include <memory>

#include "Application.h"
#include "KeyedObject.h"
#include "Work.h"

#include "KeyedObject.h"
namespace pvol
{
KEYED_OBJECT_POINTER(Rendering)
}

#include "Camera.h"
#include "RenderingSet.h"
#include "Datasets.h"
#include "Visualization.h"
#include "Pixel.h"

namespace pvol
{
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
	}

	void AddLocalPixels(Pixel *, int);	// Add to local FB from received send buffer
	void AddLocalPixel(Pixel *);	      // Add to local FB from received send buffer
	void AddLocalPixel(RayList *, int); // Add to local FB from terminated ray

	bool IsLocal();
	void SaveImage(string, int);

	virtual int serialSize();
	virtual unsigned char *serialize(unsigned char *);
	virtual unsigned char *deserialize(unsigned char *);

	float *GetPixels() { return framebuffer; }

protected:

	// weak pointers!

	RenderingSetW  renderingSet;
	VisualizationW visualization;
	CameraW    		 camera;
	DatasetsW  		 datasets;

	int owner;
	int accumulation_knt;

	float *framebuffer;

	int width, height;
};
}
