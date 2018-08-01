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

#include <iostream>

#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <cstdlib>
#include <math.h>

#include <dtypes.h>
#include <Application.h>
#include "OSPRayRenderer.h"

using namespace gxy;


class SampleMsg : public Work
{
public:
	SampleMsg(VolumeP v, ParticlesP p) : SampleMsg(2*sizeof(Key))
	{
		((Key *)contents->get())[0] = v->getkey();
		((Key *)contents->get())[1] = p->getkey();
	}
	~SampleMsg() {}

	WORK_CLASS(SampleMsg, true /* bcast */)

public:
	bool CollectiveAction(MPI_Comm c, bool s)
	{
		Key* keys = (Key *)contents->get();
		VolumeP v = Volume::Cast(KeyedDataObject::GetByKey(keys[0]));
		ParticlesP p = Particles::Cast(KeyedDataObject::GetByKey(keys[1]));

		p->CopyPartitioning(v);

		float deltaX, deltaY, deltaZ;
		v->get_deltas(deltaX, deltaY, deltaZ);

		float ox, oy, oz;
		v->get_local_origin(ox, oy, oz);

		int nx, ny, nz;
		v->get_local_counts(nx, ny, nz);

		int xstep = 1;
		int ystep = nx;
		int zstep = nx * ny;

		float *fsamples = (float *)v->get_samples();

		p->allocate(100);
		Particle *particle = p->get_samples();

		for (int i = 0; i < 100; i++)
		{
			float x = ((float)rand() / RAND_MAX) * (nx - 1);
			float y = ((float)rand() / RAND_MAX) * (ny - 1);
			float z = ((float)rand() / RAND_MAX) * (nz - 1);

			int ix = (int)x;
			int iy = (int)y;
			int iz = (int)z;

			float dx = x - ix;
			float dy = y - iy;
			float dz = z - iz;

			int v000 = (ix + 0) * xstep + (iy + 0) * ystep + (iz + 0) * zstep;
			int v001 = (ix + 0) * xstep + (iy + 0) * ystep + (iz + 1) * zstep;
			int v010 = (ix + 0) * xstep + (iy + 1) * ystep + (iz + 0) * zstep;
			int v011 = (ix + 0) * xstep + (iy + 1) * ystep + (iz + 1) * zstep;
			int v100 = (ix + 1) * xstep + (iy + 0) * ystep + (iz + 0) * zstep;
			int v101 = (ix + 1) * xstep + (iy + 0) * ystep + (iz + 1) * zstep;
			int v110 = (ix + 1) * xstep + (iy + 1) * ystep + (iz + 0) * zstep;
			int v111 = (ix + 1) * xstep + (iy + 1) * ystep + (iz + 1) * zstep;

			float b000 = (1.0 - dx) * (1.0 - dy) * (1.0 - dz);
			float b001 = (1.0 - dx) * (1.0 - dy) * (dz);
			float b010 = (1.0 - dx) * (dy) * (1.0 - dz);
			float b011 = (1.0 - dx) * (dy) * (dz);
			float b100 = (dx) * (1.0 - dy) * (1.0 - dz);
			float b101 = (dx) * (1.0 - dy) * (dz);
			float b110 = (dx) * (dy) * (1.0 - dz);
			float b111 = (dx) * (dy) * (dz);

			particle->u.value = fsamples[v000]*b000 +
													fsamples[v001]*b001 +
													fsamples[v010]*b010 +
												  fsamples[v011]*b011 +
										 			fsamples[v100]*b100 +
										 			fsamples[v101]*b101 +
										 			fsamples[v110]*b110 +
										 			fsamples[v111]*b111;

			particle->xyz.x = ox + x*deltaX;
			particle->xyz.y = oy + y*deltaY;
			particle->xyz.z = oz + z*deltaZ;

			particle ++;
		}

		std::cerr << "XXXX" << std::endl;
		return false;
	}
};

WORK_CLASS_TYPE(SampleMsg)

int
main(int argc, char * argv[])
{
	Application theApplication(&argc, &argv);

	theApplication.Start();

	if (getenv("DEBUG"))
	{
		setup_debugger(argv[0]);
		debugger(getenv("DEBUG"));
	}

	OSPRayRenderer *theRenderer = new OSPRayRenderer(&argc, &argv);

	theApplication.Run();

  int r = theApplication.GetRank();
  int s = theApplication.GetSize();

	SampleMsg::Register();

	if (r == 0)
	{
		// create empty distributed container for volume data
		VolumeP volume = theRenderer->NewVolume();
		// import data to all processes, smartly distributes volume across processses
		// this import defines the partitioning of the data across processses
		// if subsequent Import commands have a different partition, an error will be thrown
		volume->Import(argv[1]);

		// create empty distributed container for particles
		// particle partitioning will ma
		ParticlesP samples = theRenderer->NewParticles();

		// define action to perform on volume (see SampleMsg above)
		SampleMsg *smsg = new SampleMsg(volume, samples);
		smsg->Broadcast(true, true);

		theRenderer->SetALight(1.0, 2.0, 3.0);
		theRenderer->SetLightingModel(0.4, 0.6);
		theRenderer->SetRaycastLightingModel(false, 0, 0.0);
		theRenderer->Commit();

		DatasetsP theDatasets = Datasets::NewP();
		theDatasets->Insert("samples", samples);
		theDatasets->Commit();

		vector<CameraP> theCameras;
		for (int i = 0; i < 20; i++)
		{
			CameraP cam = Camera::NewP();

			cam->set_viewup(0.0, 1.0, 0.0);
			cam->set_angle_of_view(45.0);

			float angle = 2*3.1415926*(i / 20.0);

			float vpx = 8.0 * cos(angle);
			float vpy = 8.0 * sin(angle);

			cam->set_viewpoint(vpx, vpy, 0.0);
			cam->set_viewdirection(-vpx, -vpy, 0.0);

			cam->Commit();
			theCameras.push_back(cam);
		}

		ParticlesVisP pvis = theRenderer->NewParticlesVis();
		pvis->SetName("samples");
		pvis->SetRadius(0.02);
		pvis->Commit(theDatasets);

		VisualizationP v = theRenderer->NewVisualization();
		v->AddVis(pvis);
		v->Commit(theDatasets);

		RenderingSetP theRenderingSet = RenderingSet::NewP();

		int indx = 0;
		for (auto c : theCameras)
		{
			RenderingP theRendering = Rendering::NewP();
			theRendering->SetTheOwner((indx++) % s);
			theRendering->SetTheSize(1920, 1080);
			theRendering->SetTheCamera(c);
			theRendering->SetTheDatasets(theDatasets);
			theRendering->SetTheVisualization(v);
			theRendering->SetTheRenderingSet(theRenderingSet);
			theRendering->Commit();
			theRenderingSet->AddRendering(theRendering);
		}

		theRenderingSet->Commit();

		theRenderer->Render(theRenderingSet);
		theRenderingSet->WaitForDone();
		theRenderingSet->SaveImages(string("samples"));

		theApplication.QuitApplication();
	}

	theApplication.Wait();
}
