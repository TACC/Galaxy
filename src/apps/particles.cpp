#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <fcntl.h>

#include "dtypes.h"

#include "../rapidjson/document.h"
#include "../rapidjson/filestream.h"
#include "../rapidjson/prettywriter.h"
#include "../rapidjson/stringbuffer.h"

using namespace gxy;
using namespace rapidjson;
using namespace std;

float frand()
{
  return ((float)rand() / RAND_MAX);
}

void
syntax(char *a)
{
	cerr << "syntax: " << a << " [options] outfile\n";
	cerr << "options:\n";
	cerr << "    -v vfile               .vol file to get BB info (-1 -> 1)\n";
  cerr << "    -m x X y Y z Z         bounding box\n";
	cerr << "    -n nparticles          number of particles (1000)\n";
	cerr << "    -R                     add random value\n";
	exit(1);
}

int
main(int argc, char *argv[])
{

	string oname = "";
	string volname = "";
  int nparticles = 1000;
  bool mm_set = false;
  vec3f mm, MM;
	bool random_v = false;

  srand(time(NULL));

	for (int i = 1; i < argc; i++)
	{
		if (argv[i][0] == '-')
			switch(argv[i][1])
			{
        case '-': syntax(argv[0]);
				case 'n': nparticles   = atoi(argv[++i]); break;
        case 'v': volname = argv[++i]; break;
				case 'R': random_v = true; break;
        case 'm': 
          {
            float mx = atof(argv[++i]);
            float Mx = atof(argv[++i]);
            float my = atof(argv[++i]);
            float My = atof(argv[++i]);
            float mz = atof(argv[++i]);
            float Mz = atof(argv[++i]);
            mm = {mx, my, mz};
            MM = {Mx, My, Mz};
            mm_set = true;
          }
          break;
				default:
					syntax(argv[0]);
			}
		else if (oname == "") oname = argv[i];
		else syntax(argv[0]);
	}
	
  if (oname == "")
		syntax(argv[0]);

	if (volname != "")
	{
		ifstream in(volname);

		string t;
		in >> t;

		vec3i gc;
		vec3f d, go, ge;

		in >> go.x >> go.y >> go.z;
		in >> gc.x >> gc.y >> gc.z;
		in >> d.x >> d.y >> d.z;

    mm = {go.x, go.y, go.z};
    MM = {go.x + (gc.x-1)*d.x, go.y + (gc.y-1)*d.y, go.z + (gc.z-1)*d.z};
	}
	else if (!mm_set)
	{
    mm = {-1, -1, -1};
    MM = {1, 1, 1};
  }

	int psize = random_v ? 4 : 3;
  float *particles = new float[psize*nparticles];
	float *p = particles;
  
  if (nparticles == 1)
  {
    *p++ = (MM.x + mm.x) / 2.0;
    *p++ = (MM.x + mm.x) / 2.0;
    *p++ = (MM.x + mm.x) / 2.0;
    if (random_v)
      *p++ = frand();
  }
  else
    for (int i = 0; i < nparticles; i++)
    {
      *p++ = mm.x + frand()*(MM.x - mm.x);
      *p++ = mm.y + frand()*(MM.y - mm.y);
      *p++ = mm.z + frand()*(MM.z - mm.z);
      if (random_v)
        *p++ = frand();
    }

  char buf[256];
  int fd = open(oname.c_str(), O_TRUNC | O_WRONLY | O_CREAT, 0644);
  write(fd, particles, psize*nparticles*sizeof(float));
  close(fd);

  delete[] particles;
}
