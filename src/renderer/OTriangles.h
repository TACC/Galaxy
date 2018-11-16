#include "Application.h"
#include "Triangles.h"
#include "OSPRayObject.h"

namespace gxy
{

OBJECT_POINTER_TYPES(OTriangles)

class OTriangles : public OSPRayObject
{
  GALAXY_OBJECT(OTriangles)

public:
  static OTrianglesP NewP(TrianglesP p) { return OTriangles::Cast(std::shared_ptr<OTriangles>(new OTriangles(p))); }

private:
  OTriangles(TrianglesP);

private:
  TrianglesP triangles;
};

}

