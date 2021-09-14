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

#include <math.h>
#include <mpi.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "dtypes.h"

#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"

using namespace gxy;
using namespace rapidjson;
using namespace std;

#include "Partitioning.h"

float frand()
{
  return ((float)rand() / RAND_MAX);
}

struct Particle
{
  vec3f xyz;
  float t;
};

void
syntax(char *a)
{
  cerr << "syntax: " << a << " [options] pfile ifile obase" << endl;
  cerr << "options:" << endl;
  cerr << "    -m x X y Y z Z         bounding box (scan the points)" << endl;
  cerr << "    -s npartitions         number of partitions (1)" << endl;
  cerr << "    -r radius              max particle radius (0)" << endl;
  cerr << "    -v vfile               .vol file for BB info" << endl;
  cerr << "    -V                     input has v per point (no)" << endl;
	cerr << "    -P                     set v value to owning partition (no)" << endl;
  exit(1);
}

int fd;
unsigned char *buffer;
size_t totsize;
size_t remaining;
size_t buffersz;
int itemsize;

#define POINTS_PER_BUFFER  1000000

void
opn(string iname)
{
  struct stat info;
  if (stat(iname.c_str(), &info))
  {
    cerr << "cannot stat " << iname << endl;
    exit(1);
  }

  totsize = info.st_size;
  remaining = totsize;

  buffersz = POINTS_PER_BUFFER * itemsize;
  buffer = (unsigned char *)malloc(buffersz);
  if (! buffer)
  {
    cerr << "error allocating buffer" << endl;
    exit(1);
  }

  fd = open(iname.c_str(), O_RDONLY);
  if (fd < 0)
  {
    cerr << "cannot open " << iname << endl;
    exit(1);
  }
}

unsigned char *
nxt(int& n)
{
  if (remaining == 0)
  {
    n = 0;
    return NULL;
  }

  size_t m = remaining < buffersz ? remaining : buffersz;
  remaining -= m;

  n = m / itemsize;

  char *p = (char *)buffer;
  while (m > 0)
  {
    size_t k = read(fd, p, m);
    if (k == 0)
    {
      cerr << "read error" << endl;
      exit(1);
    }

    p += k;
    m -= k;
  }

  return buffer;
}

void
cls()
{
  close(fd);
  free(buffer);
}

void
rwnd()
{
  lseek(fd, 0, SEEK_SET);
  remaining = totsize;
}

int
main(int argc, char *argv[])
{
	MPI_Init(&argc, &argv);

  string pfile = "", obase = "", iname = "";
  float max_radius = 0.0;
	bool input_has_v = false;
	bool p_value = false;

  for (int i = 1; i < argc; i++)
    if (argv[i][0] == '-')
      switch(argv[i][1])
      {
        case 'V': input_has_v = true; break;
        case 'P': p_value = true; break;
        case 'r': max_radius  = atof(argv[++i]); break;
        default: syntax(argv[0]);
      }
    else if (pfile == "") pfile = argv[i];
    else if (iname == "") iname = argv[i];
    else if (obase == "") obase = argv[i];
    else syntax(argv[0]);

  if (pfile == "" || iname == "" || obase == "")
    syntax(argv[0]);

  PartitioningP partitioning = Partitioning::NewP();

  Document *pdoc = new Document();

  ifstream ifs(pfile);
  if (ifs)
  {
    stringstream ss;
    ss << ifs.rdbuf();

    if (pdoc->Parse<0>(ss.str().c_str()).HasParseError())
    {
      std::cerr << "Error parsing JSON file: " << pfile << "\n";
      delete pdoc;
      exit(0);
    }
  }
  else
  {
    std::cerr << "Error opening JSON file: " << pfile << "\n";
    delete pdoc;
    exit(0);
  }

  partitioning->LoadFromJSON(*pdoc);

  delete pdoc;

  int npartitions = partitioning->get_number_of_partitions();

	if (mkdir(obase.c_str(), 0755) && errno != EEXIST)
	{
		cerr << "unable to create partition directory" << endl;
		exit(1);
	}
    
  itemsize = (input_has_v ? 4 : 3)*sizeof(float);

  opn(iname);

  for (int i = 0; i < npartitions; i++)
  {
    char buf[256];
    sprintf(buf, "%s/%s-%04d.sph", obase.c_str(), obase.c_str(), i);
    int fd = open(buf, O_TRUNC | O_WRONLY | O_CREAT, 0644);
    close(fd);
  }

  int n;
  vector<Particle> *partitions = new vector<Particle>[npartitions];
  for (unsigned char *p = nxt(n); p; p = nxt(n))
  {
    for (int i = 0; i < n; i++, p += itemsize)
    {
      Particle particle;
			float *pp = (float *)p;
  
      particle.xyz.x = pp[0];
      particle.xyz.y = pp[1];
      particle.xyz.z = pp[2];

			if (p_value)
			{
				particle.t = 0;
				for (int j = 0; j < npartitions; j++)
					if (partitioning->isIn(j, particle.xyz, max_radius))
					{
						particle.t = j;
						break;
					}
			}
			else if (input_has_v)
				particle.t = pp[3];
			else
				particle.t = frand();
    
      for (int j = 0; j < npartitions; j++)
        if (partitioning->isIn(j, particle.xyz, max_radius))
				{
					partitions[j].push_back(particle);
				}
    }

    for (int i = 0; i < npartitions; i++)
    {
      char buf[256];
      sprintf(buf, "%s/%s-%04d.sph", obase.c_str(), obase.c_str(), i);
      int fd = open(buf, O_APPEND | O_WRONLY, 0644);
      int n = partitions[i].size();
      unsigned char *p = (unsigned char *)partitions[i].data();
      int l = n*sizeof(Particle);
      std::cerr << write(fd, p, l) << "\n";
      close(fd);
      partitions[i].clear();
    }
  }

  Document doc;
  doc.Parse("{}");

  Value parts(kArrayType);

  for (int i = 0; i < npartitions; i++)
  {
    Value part(kObjectType);

    char buf[256];
		sprintf(buf, "%s/%s-%04d.sph", obase.c_str(), obase.c_str(), i);
    part.AddMember("filename", Value().SetString(buf, doc.GetAllocator()), doc.GetAllocator());
    parts.PushBack(part, doc.GetAllocator());
  }

  doc.AddMember("parts", parts, doc.GetAllocator());

  StringBuffer sbuf;
  PrettyWriter<StringBuffer> writer(sbuf);
  doc.Accept(writer);

  ofstream ofs;
  ofs.open((obase + ".part").c_str(), ofstream::out);
  ofs << sbuf.GetString() << "\n";
  ofs.close();

  cls();

  MPI_Finalize();
}
