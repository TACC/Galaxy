#pragma once

#include <string>
#include <string.h>
#include <memory.h>
#include <vector>

using namespace std;

#include "OSPRayObject.h"

namespace pvol
{

KEYED_OBJECT_POINTER(Geometry)

class Box;

class Geometry : public OSPRayObject
{
  KEYED_OBJECT_SUBCLASS(Geometry, OSPRayObject)
};

}

