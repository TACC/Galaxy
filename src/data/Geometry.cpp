#include <string>
#include <string.h>
#include <memory.h>
#include <vector>

#include <ospray/ospray.h>

using namespace std;

#include "Geometry.h"
#include "Particles.h"

KEYED_OBJECT_TYPE(Geometry)

void
Geometry::Register()
{
	RegisterClass();
	Particles::Register();
}

