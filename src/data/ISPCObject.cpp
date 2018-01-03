#include <iostream>
#include <math.h>
#include <stdlib.h>
#include <string>
#include <sstream>
#include <fstream>
#include "Application.h"

#include "ISPCObject.h"
#include "ISPCObject_ispc.h"

using namespace std;

void 
ISPCObject::allocate_ispc()
{
	if (! ispc)
		ispc = ispc::ISPCObject_allocate();
}

void 
ISPCObject::initialize_ispc()
{
	ispc::ISPCObject_initialize(ispc);
}

void 
ISPCObject::destroy_ispc()
{
	if (ispc)
	{
		ispc::ISPCObject_destroy(ispc);
		ispc = NULL;
	}
}
