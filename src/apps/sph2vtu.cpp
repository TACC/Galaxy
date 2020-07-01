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

#include <iostream>
#include <math.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string>
#include <sstream>
#include <fstream>
#include <sys/stat.h>
#include <fcntl.h>

#include <vtkFloatArray.h>
#include <vtkVertex.h>
#include <vtkIdList.h>
#include <vtkPoints.h>
#include <vtkUnstructuredGrid.h>
#include <vtkXMLUnstructuredGridWriter.h>

struct XYZI
{
	float x, y, z;
	int i;
};

int
main(int argc, char **argv)
{
  struct stat info;
  if (stat(argv[1], &info))
  {
    cerr << "Cannot open " << argv[1] << "\n";
    exit(1);
  }

	int nParticles = info.st_size / sizeof(XYZI);
	if (argc == 3)
	{
		nParticles = atoi(argv[2]);
		if (nParticles > info.st_size / sizeof(XYZI))
			nParticles = info.st_size / sizeof(XYZI);
	}
	else
		nParticles = info.st_size / sizeof(XYZI);

	XYZI *xyzi = new XYZI[nParticles];
	
	int fd = open(argv[1], O_RDONLY);

	size_t sz = nParticles * sizeof(XYZI);
	unsigned char *p = (unsigned char *)xyzi;
	while(sz)
	{
		size_t n = read(fd, p, sz);
		if (n <= 0)
		{
			std::cerr << "read error\n";
			exit(0);
		}

		p += n;
		sz -= n;
	}

	std::cerr << "read done\n";

	vtkUnstructuredGrid *ug = vtkUnstructuredGrid::New();
	ug->Allocate(nParticles);

	vtkPoints *pts = vtkPoints::New();
	
	vtkIdList *ids = vtkIdList::New();
	ids->SetNumberOfIds(1);
	for (int i = 0; i < nParticles; i++)
	{
		pts->InsertNextPoint(xyzi[i].x, xyzi[i].y, xyzi[i].z);
		ids->SetId(0, i);
		ug->InsertNextCell(VTK_VERTEX, ids);
	}

	delete[] xyzi;

	ug->SetPoints(pts);
	pts->Delete();

	vtkXMLUnstructuredGridWriter *w = vtkXMLUnstructuredGridWriter::New();

  argv[1][strlen(argv[1])-4] = 0;
  char buf[256];
  sprintf(buf, "%s.vtp", argv[1]);
 
  w->SetFileName(buf);
	w->SetInputData(ug);
	w->Update();

	ug->Delete();
	w->Delete();

	exit(1);
}
