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

#include <stdlib.h>
#include <iostream>
#include <ctype.h>
#include <sstream>

#include <chrono>

#include <dtypes.h>
#include <Application.h>

#include <galaxy.h>
#include <OSPRayRenderer.h>

#ifdef WITH_VTK_RENDERER
#include <GeomRenderer.h>
#endif

#include "Cinema.h"

using namespace pvol;

using namespace std;
using namespace gxy;

char  *
timestamp()
{
  time_t timer;
  time(&timer);
  return asctime(localtime(&timer));
}


void
syntax(char *a)
{
  if (Application::GetTheApplication()->GetRank() == 0)
  {
    std::cerr << "\n";
    std::cerr << "  usage:  " << a << " [options] statefile\n";
    std::cerr << "\n";
    std::cerr << "  Options:\n";
    std::cerr << "    -F                          : save state files\n";
    std::cerr << "    -s w h                      : size of images (1920x1080)\n";
#ifdef WITH_VTK_RENDERER
  	std::cerr << "    -R                          : use raycaster (default)\n";
  	std::cerr << "    -V                          : use VTK\n";
#endif
  	std::cerr << "    -D dbg                      : debug - dbg is -1 for all, or k to debug rank k\n";
  }

	exit(1);
}

int
main(int argc, char *argv[])
{
	int w = 1920, h = 1080;
	bool saveState = false;
	string filename("");
	char* dbg = NULL;
	bool raycaster = true;

	Application theApplication(&argc, &argv);
	theApplication.Start();

	for (int i = 1; i < argc; i++)
	{
    if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--")) syntax(argv[0]);

    else if (!strcmp(argv[i], "-s")) { w = atoi(argv[++i]); h = atoi(argv[++i]); }
		else if (!strcmp(argv[i], "-F")) saveState = true;
		else if (!strcmp(argv[i], "-D")) dbg = argv[++i];
		else if (!strcmp(argv[i], "-R")) raycaster = true; 
		else if (!strcmp(argv[i], "-V")) raycaster = false;
		else if (filename == "")         filename = argv[i];
		else syntax(argv[0]);
  }

	if (filename == "")
		syntax(argv[0]);

#ifdef WITH_VTK_RENDERER
	Renderer  *theRenderer = raycaster ? (Renderer *) new OSPRayRenderer(&argc, &argv) : new GeomRenderer(&argc, &argv);
#else
	if (! raycaster)
		std::cerr << "this version does not support VTK rendering\n";

	Renderer  *theRenderer = (Renderer *) new OSPRayRenderer(&argc, &argv);
#endif

	setup_debugger(argv[0]);
	debugger(dbg);

	theApplication.Run();


	bool first = true;
	if (theApplication.GetRank() == 0)
	{
		std::cerr << "Application running: " << timestamp();

		theRenderer->Init(w, h);

		Camera::SetCurrent(theRenderer->NewCamera());
		DataModel::SetCurrent(theRenderer->NewDataModel());

		Cinema cinema(filename);

		std::cerr << "Requires " << cinema.Count() << " images per time step\n";
		cinema.setSaveState(saveState);

		if (DataModel::GetCurrent()->has_an_attachment)
		{
			int frame = 0;
			while (DataModel::GetCurrent()->WaitForTimestep())
      {
				if (first)
				{
					first = false;
					std::cerr << "first connection made: " << timestamp();
				}
				auto t1 = std::chrono::high_resolution_clock::now();
        DataModel::GetCurrent()->LoadTimestep();
				cinema.Render(frame++);
				auto t2 = std::chrono::high_resolution_clock::now();
				std::cerr << "rendering time "
						<< std::chrono::duration_cast<std::chrono::milliseconds>(t2-t1).count() << " milliseconds\n";
			}
		}
		else
		{
			auto t1 = std::chrono::high_resolution_clock::now();
			Camera::GetCurrent()->Commit();
			DataModel::GetCurrent()->Commit();
			theRenderer->Commit();
			cinema.Render(0);
			auto t2 = std::chrono::high_resolution_clock::now();
			std::cerr << "rendering time "
						<< std::chrono::duration_cast<std::chrono::milliseconds>(t2-t1).count() << " milliseconds\n";
		}

		cinema.WriteInfo();

		std::cerr << "Application finished: " << timestamp();

		theApplication.QuitApplication();
	}

	theApplication.Wait();

  delete DataModel::GetCurrent();
  delete Camera::GetCurrent();
  delete theRenderer;

	return(0);
}
