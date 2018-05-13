#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <pthread.h>


#include <dtypes.h>
#include <Application.h>
#include <Events.h>

using namespace std;
using namespace pvol;

class BcastEvent : public Event
{ 
public:
 BcastEvent() {}

protected:
  void print(ostream& o)
  {
    Event::print(o);
    o << "BCAST";
  }
};

class BcastMsg : public Work
{
public:
	BcastMsg() : BcastMsg(10*sizeof(int)) 
	{
		int *p = (int *)contents->get();
		for (int i = 0; i < 10; i++) *p++ = i + GetTheApplication()->GetRank();
	}
	~BcastMsg() {}

	WORK_CLASS(BcastMsg, true)

public:
	bool CollectiveAction(MPI_Comm s, bool isRoot)
	{
		GetTheEventTracker()->Add(new BcastEvent);
		stringstream xx;
		for (int i = 0; i < 10; i++)
			xx << i << " ";
		APP_PRINT(<< xx.str());
		return false;
	}
};

WORK_CLASS_TYPE(BcastMsg)

int
main(int argc, char * argv[])
{
	Application theApplication(&argc, &argv);
	theApplication.Start();

  setup_debugger(argv[0]);
  debugger(argv[1]);

	theApplication.Run();

	BcastMsg::Register();

	int r = theApplication.GetRank();
	int s = theApplication.GetSize();

	if (r == 0)
	{
		BcastMsg b0;
		b0.Broadcast(true, true);
		sleep(100000);
		theApplication.QuitApplication();
	}
	else
	{
		stringstream xx;
		xx << "hello " << GetTheApplication()->GetRank();
		APP_PRINT(<< xx.str());
	}

	theApplication.Wait();
}
