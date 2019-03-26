#include "Application.h"
#include "Volume.h"
#include "OSPRayObject.h"

namespace gxy
{

OBJECT_POINTER_TYPES(OVolume)

class OVolume : public OSPRayObject
{
  GALAXY_OBJECT(OVolume)

public:
  static OVolumeP NewP(VolumeP p) { return OVolume::Cast(std::shared_ptr<OVolume>(new OVolume(p))); }
  ~OVolume() { std::cerr << "OVolume dtor\n"; }

private:
  OVolume(VolumeP);

private:
  VolumeP volume;
};

}

