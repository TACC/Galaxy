#include "Camera.h"
#include "KeyedDataObject.h"
#include "Rendering.h"
#include "RenderingSet.h"
#include "Vis.h"
#include "Visualization.h"

namespace gxy
{
void
RegisterDataObjects()
{
	KeyedDataObject::Register();
	Vis::Register();
	Visualization::Register();
}
}