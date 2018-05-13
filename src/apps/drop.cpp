#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>

#include <Application.h>

using namespace pvol;

#include "TestObject.h"

int
main(int argc, char * argv[])
{
	Application theApplication(&argc, &argv);
	theApplication.Start();

  setup_debugger(argv[0]);
  debugger(argv[1]);

	theApplication.Run();

	TestObject::Register();

	std::cerr << "XX " << theApplication.GetRank() << "\n";

	if (theApplication.GetRank() == 0)
	{
		{
			TestObjectP top = TestObject::NewP();
			top->doit();
			top->Drop();
		}

		sleep(4);
		theApplication.QuitApplication();
	}

	theApplication.Wait();
}
