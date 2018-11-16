#include "OTriangles.h"

using namespace gxy;

OTriangles::OTriangles(TrianglesP t)
{
  triangles = t;

  int n_triangles = triangles->GetNumberOfTriangles();
  int n_vertices = triangles->GetNumberOfVertices();
  if (n_triangles > 0)
  {
    float *cptr, *colors = new float[n_vertices * 4];
    cptr = colors;
    for (int i = 0; i < n_vertices; i++)
    {
      *cptr++ = 173.0 / 255.0;
      *cptr++ = 224.0 / 255.0;
      *cptr++ = 255.0 / 255.0;
      *cptr++ = 1.0;
    }
    
    OSPData pdata = ospNewData(n_vertices, OSP_FLOAT3, triangles->GetVertices(), OSP_DATA_SHARED_BUFFER);
    ospCommit(pdata);

    OSPData ndata = ospNewData(n_vertices, OSP_FLOAT3, triangles->GetNormals(), OSP_DATA_SHARED_BUFFER);
    ospCommit(ndata);
   
    OSPData tdata = ospNewData(n_triangles, OSP_INT3, triangles->GetTriangles(), OSP_DATA_SHARED_BUFFER);
    ospCommit(tdata);
 
    OSPData cdata = ospNewData(n_vertices, OSP_FLOAT4, colors);
    ospCommit(cdata);
    delete[] colors;

    OSPGeometry ospg = ospNewGeometry("triangles");

    ospSetData(ospg, "vertex", pdata);
    ospSetData(ospg, "vertex.normal", ndata);
    ospSetData(ospg, "index", tdata);
    ospSetData(ospg, "color", cdata);

    ospCommit(ospg);

    theOSPRayObject = (OSPObject)ospg;
  }
  else
    theOSPRayObject = nullptr;
}
