#pragma once

#include <string>
#include <string.h>
#include <memory.h>
#include <vector>

#include <ospray/ospray.h>

#include "KeyedDataObject.h"
#include "OSPUtil.h"

namespace pvol
{
KEYED_OBJECT_POINTER(OSPRayObject)

class Box;

class OSPRayObject : public KeyedDataObject
{
  KEYED_OBJECT_SUBCLASS(OSPRayObject, KeyedDataObject)

public:
	virtual void initialize();
	virtual ~OSPRayObject();

	OSPObject GetOSP() { return theOSPRayObject; }
	void      *GetOSP_IE() { return osp_util::GetIE((void *)theOSPRayObject); }

protected:
	OSPObject theOSPRayObject;
};
}
