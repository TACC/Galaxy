#include "Application.h"
#include "Particles.h"
#include "OSPRayObject.h"


namespace gxy
{

OBJECT_POINTER_TYPES(OParticles)

class OParticles : public OSPRayObject
{
  GALAXY_OBJECT(OParticles) 

public:
  static OParticlesP NewP(ParticlesP p) { return OParticles::Cast(std::shared_ptr<OParticles>(new OParticles(p))); }
  
private:
  OParticles(ParticlesP);

private:
  ParticlesP particles;
};

}

