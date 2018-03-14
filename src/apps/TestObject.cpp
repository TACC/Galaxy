#include "Application.h"
#include "TestObject.h"

using namespace pvol;

WORK_CLASS_TYPE(TestObject::DoitMsg);
KEYED_OBJECT_TYPE(TestObject);

void TestObject::initialize() { APP_PRINT(<< "TestObject initialize"); }
TestObject::~TestObject() { APP_PRINT(<< "TestObject dtor"); }

void
TestObject::Register()
{
	RegisterClass();
	DoitMsg::Register();
};
