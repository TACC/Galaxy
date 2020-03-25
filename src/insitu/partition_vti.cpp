

#include <stdio.h>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/filewritestream.h"

using namespace rapidjson;

#include <vtkImageData.h>
#include <vtkXMLImageDataReader.h>
#include <vtkXMLImageDataWriter.h>
#include <vtkExtractVOI.h>
#include <vtkPointData.h>
#include <vtkAbstractArray.h>
#include <vtkFloatArray.h>
#include <vtkDoubleArray.h>

void syntax(char *a)
{
  cerr << "syntax: " << a << " n imagedata\n";
  exit(0);
}

struct vec3i
{
  int x, y, z;
};

int pindex(vec3i g, int i, int j, int k)
{
  return (k * g.y * g.x) + (j * g.x) + i;
}

struct part
{
  vec3i ijk;
  vec3i counts;
  vec3i gcounts;
  vec3i offsets;
  vec3i goffsets;
};

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

std::vector<part> 
partition(int ijk, vec3i factors, vec3i grid)
{
  // grid.[xyz] is index of top and *not* the count, so 
  // the *number* of non-ghost samples is that 
  // plus one (to get the count) minus two (the ghost points)

  int ni = grid.x - 1;
  int nj = grid.y - 1;
  int nk = grid.z - 1;

  int di = ni / factors.x;
  int dj = nj / factors.y;
  int dk = nk / factors.z;

  std::vector<part> parts;
  for (int k = 0; k < factors.z; k++)
    for (int j = 0; j < factors.y; j++)
      for (int i = 0; i < factors.x; i++)
      {
        part p;

        p.ijk.x = i;
        p.ijk.y = j;
        p.ijk.z = k;


        p.offsets.x = 1 + i*di;
        p.offsets.y = 1 + j*dj;
        p.offsets.z = 1 + k*dk;

        p.goffsets.x = p.offsets.x - 1;
        p.goffsets.y = p.offsets.y - 1;
        p.goffsets.z = p.offsets.z - 1;

        p.counts.x = 1 + ((i == (factors.x-1)) ? ni - p.offsets.x : di);
        p.counts.y = 1 + ((j == (factors.y-1)) ? nj - p.offsets.y : dj);
        p.counts.z = 1 + ((k == (factors.z-1)) ? nk - p.offsets.z : dk);

        p.gcounts.x = p.counts.x + 2;
        p.gcounts.y = p.counts.y + 2;
        p.gcounts.z = p.counts.z + 2;
    
        parts.push_back(p);
      }

  return parts;
}


int
main(int argc, char **argv)
{
  if (argc != 3)
    syntax(argv[0]);

  std::string filename = argv[2];

  std::string dirname, froot;
  size_t lastslash = filename.find_last_of("/"); 
  size_t lastdot = filename.find_last_of("."); 

  if (lastslash == std::string::npos)
  {
    lastslash = 0;
    dirname = ".";
    froot = filename.substr(0, lastdot); 
  }
  else
  {
    dirname = filename.substr(0, lastslash); 
    froot = filename.substr(lastslash+1, (lastdot - lastslash) - 1); 
  }
  std::cerr << "dirname: " << dirname << " froot: " << froot << "\n";


  int nparts = atoi(argv[1]);

  vtkXMLImageDataReader *rdr = vtkXMLImageDataReader::New();
  rdr->SetFileName(argv[2]);
  rdr->Update();

  vtkImageData *src = rdr->GetOutput();
  
  int extent[6];
  src->GetExtent(extent);

  double origin[3];
  src->GetOrigin(origin);

  double spacing[3];
  src->GetSpacing(spacing);

  vec3i grid = {extent[1] - extent[0], extent[3] - extent[2], extent[5] - extent[4]};
  
  vec3i factors;
  factor(nparts, factors);

  vtkExtractVOI *ext = vtkExtractVOI::New();
  ext->SetInputData(src);

  vtkXMLImageDataWriter *wrtr = vtkXMLImageDataWriter::New();
  wrtr->SetInputConnection(ext->GetOutputPort());

  Document root;
  root.Parse("{}");

  Value vextent(kArrayType);
  for (int i = 0; i < 6; i++)
    vextent.PushBack(Value(extent[i]), root.GetAllocator());

  Value vpartitions(kArrayType);
  vpartitions.PushBack(Value(factors.x), root.GetAllocator());
  vpartitions.PushBack(Value(factors.y), root.GetAllocator());
  vpartitions.PushBack(Value(factors.z), root.GetAllocator());

  Value vorigin(kArrayType), vspacing(kArrayType);
  for (int i = 0; i < 3; i++)
  {
    vorigin.PushBack(Value((float)origin[i]), root.GetAllocator());
    vspacing.PushBack(Value((float)spacing[i]), root.GetAllocator());
  }

  root.AddMember("partition grid", vpartitions, root.GetAllocator());
  root.AddMember("origin", vorigin, root.GetAllocator());
  root.AddMember("spacing", vspacing, root.GetAllocator());
  root.AddMember("extent", vextent, root.GetAllocator());

  Value vars(kArrayType);

  vtkPointData *pd = src->GetPointData();
  for (int i = 0; i < pd->GetNumberOfArrays(); i++)
  {
    const char *n = pd->GetArrayName(i);
    vtkAbstractArray *a = pd->GetAbstractArray(i);

    int nc = a->GetNumberOfComponents();
    if ((nc == 1 || nc == 3) && (vtkFloatArray::SafeDownCast(a) || vtkDoubleArray::SafeDownCast(a)))
    {
      Value var(kObjectType);
      var.AddMember("name", Value().SetString(n, root.GetAllocator()), root.GetAllocator());
      var.AddMember("vector", Value(nc == 3), root.GetAllocator());
      vars.PushBack(var, root.GetAllocator());
    }
  }

  root.AddMember("variables", vars, root.GetAllocator());

  Value doc(kArrayType);

  std::vector<part> parts = partition(nparts, factors, grid);
  int pnum = 0;
  for (auto p : parts)
  {
    Value pdoc(kObjectType);

#if 0
    std::cerr << "===========\n";
    std::cerr << "ijk      " << p.ijk.x << " " << p.ijk.y << " " << p.ijk.z << "\n";
    std::cerr << "counts   " << p.counts.x << " " << p.counts.y << " " << p.counts.z << "\n";
    std::cerr << "gcounts  " << p.gcounts.x << " " << p.gcounts.y << " " << p.gcounts.z << "\n";
    std::cerr << "offsets  " << p.offsets.x << " " << p.offsets.y << " " << p.offsets.z << "\n";
    std::cerr << "goffsets " << p.goffsets.x << " " << p.goffsets.y << " " << p.goffsets.z << "\n";
#endif

    int voi[] = {
      extent[0] + p.goffsets.x,
      extent[0] + (p.goffsets.x + p.gcounts.x) - 1,
      extent[2] + p.goffsets.y,
      extent[2] + (p.goffsets.y + p.gcounts.y) - 1,
      extent[4] + p.goffsets.z,
      extent[4] + (p.goffsets.z + p.gcounts.z) - 1,
    };

#if 0
    std::cerr << voi[0] << " " << 
                 voi[1] << " " << 
                 voi[2] << " " << 
                 voi[3] << " " << 
                 voi[4] << " " << 
                 voi[5] << "\n";
#endif

    Value ghosts(kArrayType);

    int l = p.goffsets.x != p.offsets.x;
    int u = (p.goffsets.x + p.gcounts.x) != (p.offsets.x + p.counts.x);
    ghosts.PushBack(Value(l), root.GetAllocator());
    ghosts.PushBack(Value(u), root.GetAllocator());

    l = p.goffsets.y != p.offsets.y;
    u = (p.goffsets.y + p.gcounts.y) != (p.offsets.y + p.counts.y);
    ghosts.PushBack(Value(l), root.GetAllocator());
    ghosts.PushBack(Value(u), root.GetAllocator());

    l = p.goffsets.z != p.offsets.z;
    u = (p.goffsets.z + p.gcounts.z) != (p.offsets.z + p.counts.z);
    ghosts.PushBack(Value(l), root.GetAllocator());
    ghosts.PushBack(Value(u), root.GetAllocator());

    pdoc.AddMember("ghosts", ghosts, root.GetAllocator());

    Value nbrs(kArrayType);

    l = p.ijk.x == 0 ? -1 : pindex(factors, p.ijk.x-1, p.ijk.y, p.ijk.z);
    u = p.ijk.x == (factors.x-1) ? -1 : pindex(factors, p.ijk.x+1, p.ijk.y, p.ijk.z);
    nbrs.PushBack(Value(l), root.GetAllocator());
    nbrs.PushBack(Value(u), root.GetAllocator());

    l = p.ijk.y == 0 ? -1 : pindex(factors, p.ijk.x, p.ijk.y-1, p.ijk.z);
    u = p.ijk.y == (factors.y-1) ? -1 : pindex(factors, p.ijk.x, p.ijk.y+1, p.ijk.z);
    nbrs.PushBack(Value(l), root.GetAllocator());
    nbrs.PushBack(Value(u), root.GetAllocator());

    l = p.ijk.z == 0 ? -1 : pindex(factors, p.ijk.x, p.ijk.y, p.ijk.z-1);
    u = p.ijk.z == (factors.z-1) ? -1 : pindex(factors, p.ijk.x, p.ijk.y, p.ijk.z+1);
    nbrs.PushBack(Value(l), root.GetAllocator());
    nbrs.PushBack(Value(u), root.GetAllocator());

    pdoc.AddMember("neighbors", nbrs, root.GetAllocator());

    Value extent(kArrayType);
    for (int i = 0; i < 6; i++)
      extent.PushBack(Value(voi[i]), root.GetAllocator());

    pdoc.AddMember("extent", extent, root.GetAllocator());

    char oname[1000];
    sprintf(oname, "%s_%d_%d_%d.vti", froot.c_str(), p.ijk.x, p.ijk.y, p.ijk.z);

    pdoc.AddMember("file", Value().SetString(oname, root.GetAllocator()), root.GetAllocator());
    doc.PushBack(pdoc, root.GetAllocator());

    ext->SetVOI(voi);
    wrtr->SetFileName((dirname + "/" + oname).c_str());
    wrtr->Write();

    std::cerr << "partition " << pnum++ << " done\n";
  }

  root.AddMember("partitions", doc, root.GetAllocator());

  FILE *fp = fopen((dirname + "/" + froot + ".json").c_str(), "wb");
  char wbuf[65536];
  FileWriteStream os(fp, wbuf, sizeof(wbuf));
  PrettyWriter<FileWriteStream> writer(os);
  root.Accept(writer);
  fclose(fp);

  wrtr->Delete();
  ext->Delete();
  rdr->Delete();
}
