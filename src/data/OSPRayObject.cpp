#include <string>
#include <string.h>
#include <memory.h>
#include <vector>

#include <ospray/ospray.h>

using namespace std;

#include "OSPRayObject.h"
#include "Geometry.h"
#include "Volume.h"

KEYED_OBJECT_TYPE(OSPRayObject)

void
OSPRayObject::initialize()
{
	theOSPRayObject = NULL;
  super::initialize();
}

OSPRayObject::~OSPRayObject()
{
	if (theOSPRayObject)
		ospRelease((OSPObject)theOSPRayObject);
}

void
OSPRayObject::Register()
{
	RegisterClass();
	Volume::Register();
	Geometry::Register();
}

