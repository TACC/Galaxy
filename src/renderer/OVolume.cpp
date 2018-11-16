#include "OVolume.h"

using namespace gxy;

OVolume::OVolume(VolumeP v)
{
  volume = v;

  OSPVolume ospv = ospNewVolume("shared_structured_volume");
  
  osp::vec3i counts;
  volume->get_ghosted_local_counts(counts.x, counts.y, counts.z);

  osp::vec3f origin, spacing;
  volume->get_ghosted_local_origin(origin.x, origin.y, origin.z);
  volume->get_deltas(spacing.x, spacing.y, spacing.z);
  
  OSPData data = ospNewData(counts.x*counts.y*counts.z, 
    volume->isFloat() ? OSP_FLOAT : OSP_UCHAR, (void *)volume->get_samples(), OSP_DATA_SHARED_BUFFER);
  ospCommit(data);
  
  ospSetObject(ospv, "voxelData", data);
  ospSetVec3i(ospv, "dimensions", counts);
  ospSetVec3f(ospv, "gridOrigin", origin);
  ospSetVec3f(ospv, "gridSpacing", spacing);
  ospSetString(ospv, "voxelType", volume->isFloat() ? "float" : "uchar");
  ospSetObject(ospv, "transferFunction", ospNewTransferFunction("piecewise_linear"));
  ospSetf(ospv, "samplingRate", 1.0);
  
  ospCommit(ospv);
  
  theOSPRayObject = ospv;
}
