#include <string>
#include <string.h>
#include <memory.h>
#include <vector>

#include <ospray/ospray.h>

#include "Geometry.h"
#include "Particles.h"

using namespace std;

namespace pvol
{
KEYED_OBJECT_TYPE(Geometry)

void
Geometry::Register()
{
	RegisterClass();
	Particles::Register();
}

}