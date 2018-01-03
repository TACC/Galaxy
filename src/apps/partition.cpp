#include <math.h>
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

#include "../rapidjson/document.h"
#include "../rapidjson/filestream.h"
#include "../rapidjson/prettywriter.h"
#include "../rapidjson/stringbuffer.h"

using namespace rapidjson;
using namespace std;

float frand()
{
  return ((float)rand() / RAND_MAX);
}

struct Particle
{
  vec3f xyz;
  float t;
};

// The following matches the implicit partitioning performed in Volume

static void
factor(int ijk, vec3i &factors)
{
  // If ijk is prime, 1, 1, ijk will be chosen, i+j+k == ijk+2

  if (ijk == 1)
  {
    factors.x = factors.y = factors.z = 1;
    return;
  }

  int mm = ijk+3;
  for (int i = 1; i <= ijk>>1; i++)
  {
    int jk = ijk / i;
    if (ijk == (i * jk))
    {
      for (int j = 1; j <= jk>>1; j++)
      {
        int k = jk / j;
        if (jk == (j * k))
        {
          int m = i + j + k;
          if (m < mm)
          {
            mm = m;
            factors.x = i;
            factors.y = j;
            factors.z = k;
          }
        }
      }
    }
  }
}

struct part
{
  vec3i ijk;
  vec3i counts;
  vec3i gcounts;
  vec3i offsets;
  vec3i goffsets;
};

part *
partition(int ijk, vec3i factors, vec3i grid)
{
  int ni = grid.x - 2;
  int nj = grid.y - 2;
  int nk = grid.z - 2;

  int di = ni / factors.x;
  int dj = nj / factors.y;
  int dk = nk / factors.z;

  part *parts = new part[ijk];
  part *p = parts;
  for (int k = 0; k < factors.z; k++)
    for (int j = 0; j < factors.y; j++)
      for (int i = 0; i < factors.x; i++, p++)
      {
        p->ijk.x = i;
        p->ijk.y = j;
        p->ijk.z = k;

        p->offsets.x = 1 + i*di;
        p->offsets.y = 1 + j*dj;
        p->offsets.z = 1 + k*dk;

        p->goffsets.x = p->offsets.x - 1;
        p->goffsets.y = p->offsets.y - 1;
        p->goffsets.z = p->offsets.z - 1;

        p->counts.x = 1 + ((i == (factors.x-1)) ? ni - p->offsets.x : di);
        p->counts.y = 1 + ((j == (factors.y-1)) ? nj - p->offsets.y : dj);
        p->counts.z = 1 + ((k == (factors.z-1)) ? nk - p->offsets.z : dk);

        p->gcounts.x = p->counts.x + 2;
        p->gcounts.y = p->counts.y + 2;
        p->gcounts.z = p->counts.z + 2;
      }

  return parts;
}
bool
isIn(Particle& p, float *extent, float max_radius)
{
  bool a = ((p.xyz.x+max_radius) >= extent[0]);
  bool b = ((p.xyz.x-max_radius) < extent[1]);
  bool c = ((p.xyz.y+max_radius) >= extent[2]);
  bool d = ((p.xyz.y-max_radius) < extent[3]);
  bool e = ((p.xyz.z+max_radius) >= extent[4]);
  bool f = ((p.xyz.z-max_radius) < extent[5]);

  return a && b && c && d && e && f;
}

void
syntax(char *a)
{
  cerr << "syntax: " << a << " [options] ifile obase\n";
  cerr << "options:\n";
  cerr << "    -m x X y Y z Z         bounding box (scan the points)\n";
  cerr << "    -s npartitions         number of partitions (1)\n";
  cerr << "    -r radius              max particle radius (0)\n";
  cerr << "    -v vfile               .vol file for BB info\n";
  cerr << "    -V                     input has v per point (no)\n";
	cerr << "    -P                     set v value to owning partition (no)\n";
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
    cerr << "cannot stat " << iname << "\n";
    exit(1);
  }

  totsize = info.st_size;
  remaining = totsize;

  buffersz = POINTS_PER_BUFFER * itemsize;
  buffer = (unsigned char *)malloc(buffersz);
  if (! buffer)
  {
    cerr << "error allocating buffer\n";
    exit(1);
  }

  fd = open(iname.c_str(), O_RDONLY);
  if (fd < 0)
  {
    cerr << "cannot open " << iname << "\n";
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
      cerr << "read error\n";
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
  string obase = "", iname = "";
  string volname = "";
  int npartitions = 1;
  float max_radius = 0.0;
  bool mm_set = false;
  vec3f mm, MM;
	bool input_has_v = false;
	bool p_value = false;

  for (int i = 1; i < argc; i++)
    if (argv[i][0] == '-')
      switch(argv[i][1])
      {
        case 'v': volname = argv[++i]; break;
        case 'V': input_has_v = true; break;
        case 'P': p_value = true; break;
        case 'm': 
          mm.x = atof(argv[++i]);
          MM.x = atof(argv[++i]);
          mm.y = atof(argv[++i]);
          MM.y = atof(argv[++i]);
          mm.z = atof(argv[++i]);
          MM.z = atof(argv[++i]);
          mm_set = true;
          break;
        case 's': npartitions = atoi(argv[++i]); break;
        case 'r': max_radius  = atof(argv[++i]); break;
        default: syntax(argv[0]);
      }
    else if (iname == "") iname = argv[i];
    else if (obase == "") obase = argv[i];
    else syntax(argv[0]);

  if (iname == "" || obase == "")
    syntax(argv[0]);
    
  itemsize = (input_has_v ? 4 : 3)*sizeof(float);

  vec3i gp;
  if (getenv("PARTITIONING"))
  {
    if (3 != sscanf(getenv("PARTITIONING"), "%d,%d,%d", &gp.x, &gp.y, &gp.z))
    {
      std::cerr << "Illegal PARTITIONING environment variable\n";
      exit(1);
    }
    npartitions = gp.x * gp.y * gp.z;
  }
  else
    factor(npartitions, gp);

  float *ghosted_extents = new float[npartitions * 6];
  float *extents = new float[npartitions * 6];

  opn(iname);

  if (volname != "")
  {
    ifstream in;
    in.open(volname.c_str());

    string type_string;
    in >> type_string;

    vec3f go;
    in >> go.x >> go.y >> go.z;

    vec3i gc;
    in >> gc.x >> gc.y >> gc.z;

    vec3f d;
    in >> d.x >> d.y >> d.z;

    in.close();

    part *partitions = partition(npartitions, gp, gc);

    float *e = extents;
    part *p = partitions;
    for (int i = 0; i < npartitions; i++, p++)
    {
      *e++ = go.x + p->offsets.x*d.x;
      *e++ = go.x + (p->offsets.x + p->counts.x-1)*d.x;
      *e++ = go.y + p->offsets.y*d.y;
      *e++ = go.y + (p->offsets.y + p->counts.y-1)*d.y;
      *e++ = go.z + p->offsets.z*d.z;
      *e++ = go.z + (p->offsets.z + p->counts.z-1)*d.z;
    }

    delete[] partitions;
  }
  else
  {
    if (! mm_set)
    {
      int n;
      bool first = true;
      for (unsigned char *p = nxt(n); p; p = nxt(n))
        for (int i = 0; i < n; i++, p += itemsize)
        {
          float *pp = (float *)p;

          if (first)
          {
            first = false;
            mm.x = MM.x = pp[0];
            mm.y = MM.y = pp[1];
            mm.z = MM.z = pp[2];
          }
          else
          {
            if (pp[0] < mm.x) mm.x = pp[0];
            if (pp[0] > MM.x) MM.x = pp[0];
            if (pp[1] < mm.y) mm.y = pp[1];
            if (pp[1] > MM.y) MM.y = pp[1];
            if (pp[2] < mm.z) mm.z = pp[2];
            if (pp[2] > MM.z) MM.z = pp[2];
          }
        }
      rwnd();
    }

    vec3f d((MM.x - mm.x) / gp.x, (MM.y - mm.y) / gp.y, (MM.z - mm.z) / gp.z);

    float *e = extents;

    for (int k = 0; k < gp.z; k++)
      for (int j = 0; j < gp.y; j++)
        for (int i = 0; i < gp.x; i++)
        {
          e[0] = mm.x + i*d.x;
          e[1] = (i == (gp.x-1)) ? MM.x : mm.x + (i+1)*d.x;
          e[2] = mm.y + j*d.y;
          e[3] = (j == (gp.y-1)) ? MM.y : mm.y + (j+1)*d.y;
          e[4] = mm.z + k*d.z;
          e[5] = (k == (gp.z-1)) ? MM.z : mm.z + (k+1)*d.z;

          e += 6;
        };
  }

  float *e = extents;
  float *ge = ghosted_extents;
  for (int i = 0; i < 3*npartitions; i++)
  {
    *ge++ = *e++ - max_radius;
    *ge++ = *e++ + max_radius;
  }

  for (int i = 0; i < npartitions; i++)
  {
    char buf[256];
    sprintf(buf, "%s-0-%04d.sph", obase.c_str(), i);
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
				float *e = extents;
				for (int j = 0; j < npartitions; j++, e += 6)
					if (isIn(particle, e, max_radius))
					{
						particle.t = j;
						break;
					}
			}
			else if (input_has_v)
				particle.t = pp[3];
			else
				particle.t = frand();
    
      float *ge = ghosted_extents;
      for (int j = 0; j < npartitions; j++, ge += 6)
        if (isIn(particle, ge, max_radius))
					partitions[j].push_back(particle);
    }

    for (int i = 0; i < npartitions; i++)
    {
      char buf[256];
      sprintf(buf, "%s-0-%04d.sph", obase.c_str(), i);
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

  e = extents;
  for (int i = 0; i < npartitions; i++)
  {
    Value part(kObjectType);

    Value extent(kArrayType);
    for (int j = 0; j < 6; j++)
      extent.PushBack(Value().SetDouble(*e++), doc.GetAllocator());

    part.AddMember("extent", extent, doc.GetAllocator());

    char buf[256];
    sprintf(buf, "%s-0-%04d.sph", obase.c_str(), i);
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

  delete[] ghosted_extents;
  delete[] extents;
}
