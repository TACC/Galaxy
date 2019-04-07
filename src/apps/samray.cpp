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

#define SAMPLE 1

#include <iostream>

#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <cstdlib>
#include <math.h>

#include <dtypes.h>
#include <Application.h>
#include "Renderer.h"
#include "Sampler.h"

#include <ospray/ospray.h>

int mpiRank, mpiSize;

#include "Debug.h"

using namespace gxy;
using namespace std;
using namespace rapidjson;

#define WIDTH  1920
#define HEIGHT 1080

// default values
int   width  = WIDTH;
int   height = HEIGHT;
float radius = 0.001;
int   samples_per_partition = 100;

void
syntax(char *a)
{
  cerr << "syntax: " << a << " [options] data" << endl;
  cerr << "optons:" << endl;
  cerr << "  -D            run debugger" << endl;
  cerr << "  -n nsamples   number of samples in each partition (" << samples_per_partition << ")" << endl;
  cerr << "  -s x y        window size (" << WIDTH << "x" << HEIGHT << ")" << endl;
  cerr << "  -r radius     radius of samples (" << radius << ")" << endl;
  exit(1);
}

int
main(int argc, char * argv[])
{
  string data = "";
  char *dbgarg;
  bool dbg = false;

  ospInit(&argc, (const char **)argv);

  Application theApplication(&argc, &argv);
  theApplication.Start();

  // process command line args
  for (int i = 1; i < argc; i++)
  {
    if (argv[i][0] == '-')
      switch (argv[i][1])
      {
        case 'D': dbg = true, dbgarg = argv[i] + 2; break;
        case 'n': samples_per_partition = atoi(argv[++i]); break;
        case 'r': radius = atof(argv[++i]); break;
        case 's': width = atoi(argv[++i]); height = atoi(argv[++i]); break;
        default:
          syntax(argv[0]);
      }
    else if (data == "")   data = argv[i];
    else syntax(argv[0]);
  }


  theApplication.Run();
  mpiRank = theApplication.GetRank();
  mpiSize = theApplication.GetSize();

  Sampler::Initialize();
  Renderer::Initialize();
  SamplerP  theSampler  = Sampler::NewP();
  RendererP theRenderer = Renderer::NewP();

  Document *doc = theApplication.OpenJSONFile( data ); 
  theRenderer->LoadStateFromDocument(*doc);

  srand(mpiRank);

  Debug *d = dbg ? new Debug(argv[0], false, dbgarg) : NULL;

  if (mpiRank == 0)
  {
    // BEGIN SAMPLING
    // sampling state is loaded from an external file
    DatasetsP theDatasets = Datasets::NewP();
    theDatasets->LoadFromJSON(*doc);
    theDatasets->Commit();
    
    // Create a Visualization that specifies how the volume is to be sampled...
    // No need to futz with lights, we aren't lighting
    VisualizationP vis0 = Visualization::NewP();
    vector<VisualizationP> theVisualizations = Visualization::LoadVisualizationsFromJSON(*doc);
    vis0 = theVisualizations[0];
    vis0->Commit(theDatasets);
  
    // Create a rendering set for the sampling pass...
    RenderingSetP theRenderingSet0 = RenderingSet::NewP();

    // read in a set of cameras which are used to sample the data
    vector<CameraP> theCameras;
    Camera::LoadCamerasFromJSON(*doc, theCameras);
    RenderingP theRendering0;
    for (vector<CameraP>::iterator iCam = theCameras.begin(); iCam != theCameras.end(); iCam++)
    {
        theRendering0 = Rendering::NewP();
        theRendering0->SetTheOwner(0);
        theRendering0->SetTheSize(width/4, height/4);
        theRendering0->SetTheDatasets(theDatasets);
        theRendering0->SetTheCamera(*iCam);
        theRendering0->SetTheVisualization(vis0);
        theRendering0->Commit();

        theRenderingSet0->AddRendering(theRendering0);
        theRenderingSet0->Commit();
    }

    // Creates a Particles dataset to sample into and attach it to the 
    // 'Sampler' renderer.   

    vector<string> datasets = theDatasets->GetDatasetNames(); 
    cerr << "First name " << datasets[0] << endl;
    VolumeP volume = Volume::Cast(theDatasets->Find(datasets[0]));

    ParticlesP samrays = Particles::NewP();
    samrays->CopyPartitioning(volume);
    samrays->SetDefaultColor(0.5, 0.5, 0.5, 1.0);
    theSampler->SetSamples(samrays);

    // Commit the Sampler, initiate sampling, and wait for it to be done
    theSampler->Commit();
    theSampler->Sample(theRenderingSet0);
    theRenderingSet0->WaitForDone();

    // Now the 'samrays' particle set contains the samples. Save it to the datasets
    samrays->Commit();
    theDatasets->Insert("samrays", samrays);
    theDatasets->Commit();

    // END SAMPLING


    // Now we set up a Visualization to visualize the samples.  
    // This time we'll be lighting...

    VisualizationP vis1 = Visualization::NewP();

    float light[] = {1.0, 2.0, 3.0}; int t = 1;
    Lighting *l = vis1->get_the_lights();
    l->SetLights(1, light, &t);
    l->SetK(0.8, 0.2);
    l->SetShadowFlag(false);
    l->SetAO(0, 0.0);
  
    // A ParticlesVis to render the samples

    ParticlesVisP pvis = ParticlesVis::NewP();
    pvis->SetName("samrays");
    pvis->Commit(theDatasets);
    pvis->SetRadius(radius);
    vis1->AddVis(pvis);

    vis1->Commit(theDatasets);

    // Now we set up a RenderingSet for the visualization of the particles.
    CameraP cam1 = Camera::NewP();
    cam1->set_viewup(0.0, 1.0, 0.0);
    cam1->set_angle_of_view(45.0);
    cam1->set_viewpoint(3.0, 3.0, 3.0);
    cam1->set_viewdirection(-1.0, -1.0, -1.0);
    cam1->Commit();

    RenderingSetP theRenderingSet1 = RenderingSet::NewP();

    RenderingP theRendering2 = Rendering::NewP();
    theRendering2->SetTheOwner(0);
    theRendering2->SetTheSize(width, height);
    theRendering2->SetTheDatasets(theDatasets);
    theRendering2->SetTheCamera(cam1);          
    theRendering2->SetTheVisualization(vis1);
    theRendering2->Commit();

    theRenderingSet1->AddRendering(theRendering2);
    theRenderingSet1->Commit();

    // Render, wait, and write results

    theRenderer->Render(theRenderingSet1);
    theRenderingSet1->WaitForDone();
    theRenderingSet1->SaveImages(string("samples"));

    theApplication.QuitApplication();
  }

  theApplication.Wait();
}
