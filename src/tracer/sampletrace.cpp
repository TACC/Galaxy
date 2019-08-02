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
#include <PathLinesVis.h>
#include <Renderer.h>
#include <Sampler.h>

#include "RungeKutta.h"
#include "TraceToPathLines.h"

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
  cerr << "syntax: " << a << " sampling.state rendering.state [options]" << endl;
  cerr << "optons:" << endl;
  cerr << "  -D            run debugger" << endl;
  cerr << "  -n nsamples   number of samples in each partition (" << samples_per_partition << ")" << endl;
  cerr << "  -s x y        window size (" << WIDTH << "x" << HEIGHT << ")" << endl;
  cerr << "  -r radius     radius of samples (" << radius << ")" << endl;
  cerr << "  -d factor     downsampling factor for sampler pass (0)" << endl;
  exit(1);
}

int
main(int argc, char * argv[])
{
  string sampling_state = "";
  string rendering_state = "";
  char *dbgarg;
  bool dbg = false;
  int downsample = 0;

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
        case 's': width = atoi(argv[++i]); height = atoi(argv[++i]); break;
        case 'd': downsample = atoi(argv[++i]); break;
        default:
          syntax(argv[0]);
      }
    else if (sampling_state == "")   sampling_state = argv[i];
    else if (rendering_state == "")  rendering_state = argv[i];
    else syntax(argv[0]);
  }

  if (sampling_state == "" || rendering_state == "")
    syntax(argv[0]);

  theApplication.Run();
  mpiRank = theApplication.GetRank();
  mpiSize = theApplication.GetSize();

  Sampler::Initialize();
  Renderer::Initialize();
  RungeKutta::RegisterRK();
  RegisterTraceToPathLines();

  SamplerP  theSampler  = Sampler::NewP();
  RendererP theRenderer = Renderer::NewP();

  srand(mpiRank);

  Debug *d = dbg ? new Debug(argv[0], false, dbgarg) : NULL;

  if (mpiRank == 0)
  {
    // BEGIN SAMPLING

    Document *sdoc = theApplication.OpenJSONFile(sampling_state);
    if (! sdoc)
    {
      std::cerr << "error loading sampling state file\n";
      exit(1);
    }

    Document *rdoc = theApplication.OpenJSONFile(rendering_state);
    if (! rdoc)
    {
      std::cerr << "error rendering state file\n";
      exit(1);
    }

    theSampler->LoadStateFromDocument(*sdoc);

    // sampling state is loaded from an external file
    DatasetsP theDatasets = Datasets::NewP();
    theDatasets->LoadFromJSON(*sdoc);
    theDatasets->Commit();
    
    // Create a list of sampling Visualizations that specifies how the volume is to be sampled...
    vector<VisualizationP> theSamplingVisualizations = Visualization::LoadVisualizationsFromJSON(*sdoc);

    for (auto i : theSamplingVisualizations)
      i->Commit(theDatasets);
  
    // read in a set of cameras which are used to sample the data
    vector<CameraP> theSamplingCameras;
    Camera::LoadCamerasFromJSON(*sdoc, theSamplingCameras);
    for (auto c : theSamplingCameras)
      c->Commit();

    // Create a rendering set for the sampling pass...
    RenderingSetP theSamplingRenderingSet = RenderingSet::NewP();

    for (auto v : theSamplingVisualizations)
      for (auto c : theSamplingCameras)
      {
        RenderingP r = Rendering::NewP();
        r = Rendering::NewP();
        r->SetTheOwner(0);
        r->SetTheSize(width >> downsample, height >> downsample);
        r->SetTheDatasets(theDatasets);
        r->SetTheCamera(c);
        r->SetTheVisualization(v);
        r->Commit();

        theSamplingRenderingSet->AddRendering(r);
      }

    theSamplingRenderingSet->Commit();

    // Creates a Particles dataset to sample into and attach it to the 
    // 'Sampler' renderer.   

    vector<string> datasets = theDatasets->GetDatasetNames(); 
    VolumeP volume = Volume::Cast(theDatasets->Find(datasets[0]));

    ParticlesP samples = Particles::NewP();
    samples->CopyPartitioning(volume);
    samples->SetDefaultColor(0.5, 0.5, 0.5, 1.0);
    theSampler->SetSamples(samples);

    // Commit the Sampler, initiate sampling, and wait for it to be done
    theSampler->Commit();
    theSampler->Start(theSamplingRenderingSet);
    theSamplingRenderingSet->WaitForDone();

    // Now the 'samples' particle set contains the samples. Save it to the datasets
    samples->Commit();
    theDatasets->Insert("samples", samples);
    theDatasets->Commit();

    // END SAMPLING

    RungeKuttaP rkp = RungeKutta::NewP();

    rkp->set_max_steps(100000);

    if (! rkp->SetVectorField(Volume::Cast(theDatasets->Find("vectors"))))
      exit(1);

    rkp->Commit();

    rkp->Trace(samples);
    
    PathLinesP plp = PathLines::NewP();

    TraceToPathLines(rkp, plp);
    plp->Commit();

    theDatasets->Insert("pathlines", plp);
    theDatasets->Commit();

    // And now on to rendering...

    theRenderer->LoadStateFromDocument(*rdoc);

    // Now make sure datasets are available for the rendering...

    theDatasets->LoadFromJSON(*rdoc);
    theDatasets->Commit();

    // Create a list of sampling Visualizations that specifies how the volume is to be sampled...
    vector<VisualizationP> theRenderingVisualizations = Visualization::LoadVisualizationsFromJSON(*rdoc);

    for (auto i = theRenderingVisualizations.begin(); i != theRenderingVisualizations.end(); i++)
      (*i)->Commit(theDatasets);
  
    // read in a set of cameras which are used to sample the data
    vector<CameraP> theRenderingCameras;
    Camera::LoadCamerasFromJSON(*rdoc, theRenderingCameras);
    // for (auto c = theRenderingCameras.begin(); c != theRenderingCameras.end(); c++)

    for (auto c : theRenderingCameras)
      c->Commit();

    // Create a rendering set for the sampling pass...
    RenderingSetP theRenderingRenderingSet = RenderingSet::NewP();

    for (auto v : theRenderingVisualizations)
      for (auto c : theRenderingCameras)
      {
        RenderingP r = Rendering::NewP();
        r = Rendering::NewP();
        r->SetTheOwner(0);
        r->SetTheSize(width, height);
        r->SetTheDatasets(theDatasets);
        r->SetTheCamera(c);
        r->SetTheVisualization(v);
        r->Commit();

        theRenderingRenderingSet->AddRendering(r);
      }

    theRenderingRenderingSet->Commit();

    // Render, wait, and write results

    theRenderer->Start(theRenderingRenderingSet);
    theRenderingRenderingSet->WaitForDone();
    theRenderingRenderingSet->SaveImages(string("samples"));

    theApplication.QuitApplication();
  }

  theApplication.Wait();
}
