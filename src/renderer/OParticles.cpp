#include "OParticles.h"

using namespace gxy;

OParticles::OParticles(ParticlesP p)
{
  particles = p;

  OSPGeometry ospg = ospNewGeometry("spheres");

  int n_samples;
  Particle *samples;
  particles->GetSamples(samples, n_samples);

  OSPData data = ospNewData(n_samples * sizeof(Particle), OSP_UCHAR, samples, OSP_DATA_SHARED_BUFFER);
  ospCommit(data);

  ospSetData(ospg, "spheres", data);
  ospSet1f(ospg, "radius_scale", particles->GetRadiusScale());
  ospSet1f(ospg, "radius", particles->GetRadius());
  ospSet1i(ospg, "offset_value", 12);

  srand(GetTheApplication()->GetRank());
  int r = random();
  unsigned int color = (r & 0x1 ? 0x000000ff : 0x000000A6) |
                       (r & 0x2 ? 0x0000ff00 : 0x0000A600) |
                       (r & 0x4 ? 0x00ff0000 : 0x00A60000) |
                       0xff000000;

  unsigned int *colors = new unsigned int[n_samples];
  for (int i = 0; i < n_samples; i++)
    colors[i] = color;

  OSPData clr = ospNewData(n_samples, OSP_UCHAR4, colors);
  delete[] colors;
  ospCommit(clr);
  ospSetObject(ospg, "color", clr);
  ospRelease(clr);

  ospCommit(ospg);

  theOSPRayObject = (OSPObject)ospg;
}
