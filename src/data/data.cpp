#include <iostream>
#include <vector>
#include <sstream>

using namespace std;

#include <string.h>
#include <pthread.h>

#include "Application.h"
#include "KeyedDataObject.h"

using namespace gxy;

extern void
RegisterDataObjects()
{
  KeyedDataObject::Register();
}
