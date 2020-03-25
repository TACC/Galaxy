#include <string>
#include <vector>
#include <iostream>

#include <vtkSocket.h>
#include <vtkImageData.h>
#include <vtkPointData.h>
#include <vtkFloatArray.h>

struct header
{
  size_t datasize;
  int extent[6];
  double origin[3];
  double spacing[3];
};

void
SerializeVTI(vtkSocket *skt, vtkImageData* vti, std::vector<std::string> vars)
{
  struct header hdr;

  vti->GetExtent(hdr.extent);
  vti->GetOrigin(hdr.origin);
  vti->GetSpacing(hdr.spacing);

  int point_count = ((hdr.extent[1] - hdr.extent[0])+1) 
                  * ((hdr.extent[3] - hdr.extent[2])+1)
                  * ((hdr.extent[5] - hdr.extent[4])+1);

  vtkPointData *pd = vti->GetPointData();

  hdr.datasize = 0;
  for (auto v : vars)
  {
    vtkAbstractArray *a = pd->GetArray(v.c_str());

    if (! vtkFloatArray::SafeDownCast(a))
    {
      std::cerr << "can't handle " << v << "... not float\n";
      exit(0);
    }

    int nc = a->GetNumberOfComponents();

    if (nc != 1 && nc != 3)
    {
      std::cerr << "can't handle " << v << "... gotta be scalar or 3-vector\n";
      exit(0);
    }

    hdr.datasize += v.size() + 1;
    hdr.datasize += point_count * nc * sizeof(float);
  }

  skt->Send((const void *)&hdr, sizeof(hdr));

  for (auto v : vars)
  {
    vtkAbstractArray *a = pd->GetArray(v.c_str());
    int nc = a->GetNumberOfComponents();
    skt->Send((const void *)v.c_str(), v.size()+1);
    skt->Send(a->GetVoidPointer(0), point_count * nc * sizeof(float));
  }
}

