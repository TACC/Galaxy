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
#include <vtkIntArray.h>
#include <vtkVertex.h>
#include <vtkIdList.h>
#include <vtkPoints.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkXMLPolyDataWriter.h>

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

	vtkPolyData *pd = vtkPolyData::New();
	pd->Allocate(nParticles);

	vtkPoints *pts = vtkPoints::New();

	vtkIntArray *typ = vtkIntArray::New();
	typ->SetNumberOfComponents(1);
	typ->SetNumberOfTuples(nParticles);
	typ->SetName("type");
	int *t = (int *)typ->GetVoidPointer(0);
	
	vtkIdList *ids = vtkIdList::New();
	ids->SetNumberOfIds(1);
	for (int i = 0; i < nParticles; i++)
	{
		pts->InsertNextPoint(xyzi[i].x, xyzi[i].y, xyzi[i].z);
		ids->SetId(0, i);
		pd->InsertNextCell(VTK_VERTEX, ids);
		*t++ = xyzi[i].i;
	}

	delete[] xyzi;

	pd->SetPoints(pts);
	pts->Delete();

	pd->GetPointData()->AddArray(typ);
	typ->Delete();

	vtkXMLPolyDataWriter *w = vtkXMLPolyDataWriter::New();

	argv[1][strlen(argv[1])-4] = 0;
	char buf[256];
	sprintf(buf, "%s.vtp", argv[1]);
	
	w->SetFileName(buf);
	w->SetInputData(pd);
	w->Update();

	pd->Delete();
	w->Delete();

	exit(1);
}
