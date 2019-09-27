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
#include "Interpolator.h"

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
int   sam_width  = WIDTH;
int   sam_height = HEIGHT;
int   maxsteps = 2000;
float h = 0.2;
float z = 1e-12;
float t = -1.0;
float max_i = -1;
float dt = 1.0;
int nf = 1;


void
syntax(char *a)
{
  cerr << "syntax: " << a << " sampling.state rendering.state [options]" << endl;
  cerr << "optons:" << endl;
  cerr << "  -D            run debugger" << endl;
  cerr << "  -s x y        sample size (" << WIDTH << "x" << HEIGHT << ")" << endl;
  cerr << "  -w x y        window size (" << WIDTH << "x" << HEIGHT << ")" << endl;
  cerr << "  -h h          portion of cell size to step (0.2)" << endl;
  cerr << "  -z z          termination magnitude of vectors (1e-12)" << endl;
  cerr << "  -t t          max integration time (none)" << endl;
  cerr << "  -m n          max number of steps per streamline (2000)" << endl;
  cerr << "  -P            print samples\n";
  cerr << "  -I max        scale the colormap to this to avoid hairballs (scale to max integration time)\n";
  cerr << "  -dt dt        truncate pathlines to this length in proportion of total integration time (don't truncate)\n";
  cerr << "  -nf nf        animate by moving head of pathlines from 0 to total integration time in this number of frames (1)\n";
  cerr << "  -pdata name   volume to map onto samples\n";
  cerr << "  -sdata name   volume to map onto pathlines\n";
  exit(1);
}

int
main(int argc, char * argv[])
{
  string data_state = "";
  string sampling_state = "";
  string rendering_state = "";
  string pdata = "";
  string sdata = "";
  char *dbgarg;
  bool dbg = false;
  bool printsamples = false;
  bool override_windowsize = false;
  bool override_samplesize = false;

  ospInit(&argc, (const char **)argv);

  Application theApplication(&argc, &argv);
  theApplication.Start();

  // process command line args
  for (int i = 1; i < argc; i++)
  {
    if (! strcmp(argv[i], "-D"))
    {
      dbg = true;
      dbgarg = argv[i] + 2;
    }
    else if (! strcmp(argv[i], "-m"))
    {
      maxsteps = atoi(argv[++i]);
    }
    else if (! strcmp(argv[i], "-s"))
    {
      sam_width = atoi(argv[++i]);
      sam_height = atoi(argv[++i]);
      override_samplesize = true;
    }
    else if (! strcmp(argv[i], "-w"))
    {
      width = atoi(argv[++i]);
      height = atoi(argv[++i]);
      override_windowsize = true;
    }
    else if (! strcmp(argv[i], "-z"))
    {
      z = atof(argv[++i]);
    }
    else if (! strcmp(argv[i], "-t"))
    {
      t = atof(argv[++i]);
    }
    else if (! strcmp(argv[i], "-P"))
    {
      printsamples = true;
    }
    else if (! strcmp(argv[i], "-I"))
    {
      max_i = atof(argv[++i]);
    }
    else if (! strcmp(argv[i], "-dt"))
    {
      dt = atof(argv[++i]);
    }
    else if (! strcmp(argv[i], "-nf"))
    {
      nf = atoi(argv[++i]);
    }
    else if (! strcmp(argv[i], "-pdata"))
    {
      pdata = argv[++i];
    }
    else if (! strcmp(argv[i], "-sdata"))
    {
      sdata = argv[++i];
    }
    else if (data_state == "")   
    {
      data_state = argv[i];
    }
    else if (sampling_state == "")   
    {
      sampling_state = argv[i];
    }
    else if (rendering_state == "")  
    {
      rendering_state = argv[i];
    }
    else
    {
      syntax(argv[0]);
    }
  }

  if (data_state == "" || sampling_state == "" || rendering_state == "")
    syntax(argv[0]);

  theApplication.Run();


  mpiRank = theApplication.GetRank();
  mpiSize = theApplication.GetSize();

  Debug *d = dbg ? new Debug(argv[0], false, dbgarg) : NULL;

  Sampler::Initialize();
  Renderer::Initialize();
  RungeKutta::RegisterRK();
  RegisterTraceToPathLines();

  SamplerP  theSampler  = Sampler::NewP();
  RendererP theRenderer = Renderer::NewP();

  InitializeInterpolateVolumeOntoGeometry();

  srand(mpiRank);

  if (mpiRank == 0)
  {
    Document *ddoc = theApplication.OpenJSONFile(data_state);
    if (! ddoc)
    {
      std::cerr << "error loading data state file\n";
      exit(1);
    }

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

    // load datasets 
    DatasetsP theDatasets = Datasets::NewP();
    theDatasets->LoadFromJSON(*ddoc);
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
        r->SetTheDatasets(theDatasets);
        if (override_samplesize)
        {
            std::cerr << "Overriding sampling width, height: " << sam_width << ", " << sam_height << std::endl;
            c->set_width(sam_width);
            c->set_height(sam_height);
        }
            // this call now sets size
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

    samples->Commit();

    if (sdata != "")
    {
      VolumeP vdata = Volume::Cast(theDatasets->Find(sdata));
      InterpolateVolumeOntoGeometry(samples, vdata);
      samples->Commit();
    }

    // Now the 'samples' particle set contains the samples. Save it to the datasets

    theDatasets->Insert("samples", samples);
    theDatasets->Commit();

    if (printsamples)
    {
      Particle *p; int n;
      samples->GetParticles(p, n);
      std::cout << "X,Y,Z\n";
      for (auto i = 0; i < n; i++)
        std::cout << p[i].xyz.x << "," << p[i].xyz.y << "," << p[i].xyz.z << "\n";
    }

    RungeKuttaP rkp = RungeKutta::NewP();
    rkp->set_max_steps(maxsteps);
    rkp->set_stepsize(h);
    rkp->SetMinVelocity(z);
    rkp->SetMaxIntegrationTime(t);

    if (! rkp->SetVectorField(Volume::Cast(theDatasets->Find("vectors"))))
      exit(1);

    rkp->Commit();

    rkp->Trace(samples);

    float tMax = rkp->get_maximum_integration_time();
    std::cerr << "max integration time: " << tMax << "\n";

    if (max_i == -1) max_i = tMax;
    dt = dt * max_i;

    // And now on to rendering...

    theRenderer->LoadStateFromDocument(*rdoc);

    // Now make sure datasets are available for the rendering...

    theDatasets->LoadFromJSON(*rdoc);
    theDatasets->Commit();

    // Create a list of sampling Visualizations that specifies how the volume is to be sampled...
    vector<VisualizationP> theRenderingVisualizations = Visualization::LoadVisualizationsFromJSON(*rdoc);
    // read in a set of cameras which are used to sample the data
    vector<CameraP> theRenderingCameras;
    Camera::LoadCamerasFromJSON(*rdoc, theRenderingCameras);

    for (auto c : theRenderingCameras)
      c->Commit();

    PathLinesP plp = PathLines::NewP();

    for (auto f = 0; f < nf; f ++)
    {
      float head_time = (nf == 1) ? max_i : (max_i * (f + 1)) / nf;

      // Create a rendering set for the rendering pass...
      RenderingSetP rs = RenderingSet::NewP();
    
      TraceToPathLines(rkp, plp, head_time, dt);
      plp->Commit();

      if (pdata != "")
      {
        VolumeP vdata = Volume::Cast(theDatasets->Find(pdata));
        InterpolateVolumeOntoGeometry(plp, vdata);
        plp->Commit();
      }

      theDatasets->Insert("pathlines", plp);
      theDatasets->Commit();

      for (auto v : theRenderingVisualizations)
      {
        for (auto i = 0; i < v->GetNumberOfVis(); i++)
        {
          MappedVisP mvp = MappedVis::Cast(v->GetVis(i));
          if (mvp)
            mvp->ScaleMaps(0.0, max_i);
        }
        v->Commit(theDatasets);
      }

      for (auto v : theRenderingVisualizations)
        for (auto c : theRenderingCameras)
        {
          RenderingP r = Rendering::NewP();
          r->SetTheOwner(0);
          r->SetTheDatasets(theDatasets);
          if (override_windowsize)
          {
              std::cerr << "Overriding rendering width, height: " << width << ", " << height << std::endl;
              c->set_width(width);
              c->set_height(height);
          }
          r->SetTheCamera(c);
          r->SetTheVisualization(v);
          r->Commit();

          rs->AddRendering(r);
        }

      rs->Commit();

      // Render, wait, and write results

      // if (f == (nf-1))
      {
        theRenderer->Start(rs);
        rs->WaitForDone();

        char namebuf[100];
        sprintf(namebuf, "samples-%04d", f);
        rs->SaveImages(string(namebuf));
      }
    }

    theApplication.QuitApplication();
  }

  theApplication.Wait();
}
