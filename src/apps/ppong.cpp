#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>

// #include <pvol.h>
#include <dtypes.h>
#include <Application.h>

#include <pthread.h>

pthread_mutex_t lck = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  w8 = PTHREAD_COND_INITIALIZER;

#define LOGGING 0

class PPongMsg : public Work
{
public:
	PPongMsg() : PPongMsg(100) { strcpy((char *)get(), (char *)"ppong"); /* std::cerr << "PPongMsg ctor\n"; */}
	~PPongMsg(){ /* std::cerr << GetTheApplication()->GetRank() << ": PPongMsg dtor\n"; */}
	WORK_CLASS(PPongMsg, true)

public:
	bool Action(int s)
	{
		int rank = GetTheApplication()->GetRank();
		int size = GetTheApplication()->GetSize();

		if (rank != 0)
		{ 
			PPongMsg m;
			m.Send((rank == (size-1)) ? 0 : rank + 1);
		}
		else
		{
			std::cerr << "ppong signalling\n";
			pthread_mutex_lock(&lck);
			pthread_cond_signal(&w8);
			pthread_mutex_unlock(&lck);
		}

#if LOGGING
		APP_LOG(<< rank << ": ppong");
#endif

		return false;
	}
};

class BcastMsg : public Work
{
public:
	BcastMsg() : BcastMsg(101) { strcpy((char *)get(), (char *)"bcast"); /* std::cerr << "BcastMsg ctor\n"; */ }
	~BcastMsg(){ /* std::cerr << GetTheApplication()->GetRank() << ": BcastMsg dtor\n"; */ }
	WORK_CLASS(BcastMsg, true)

public:
	bool CollectiveAction(MPI_Comm s, bool isRoot) { std:cerr << "bcast action\n"; return false;}
};

WORK_CLASS_TYPE(PPongMsg)
WORK_CLASS_TYPE(BcastMsg)

int
main(int argc, char * argv[])
{
	Application theApplication(&argc, &argv);
	theApplication.Start();

  setup_debugger(argv[0]);
  debugger(argv[1]);

	theApplication.Run();

	PPongMsg::Register();
	BcastMsg::Register();

	int r = theApplication.GetRank();
	int s = theApplication.GetSize();

	if (r == 0)
	{

		pthread_mutex_lock(&lck);

		if (s > 1)
		{
			PPongMsg m;
			m.Send(1);

			pthread_cond_wait(&w8, &lck);
			std::cerr << "lock signalled\n";
			pthread_mutex_unlock(&lck);
		}

		BcastMsg b0;
		b0.Broadcast(true);

		BcastMsg *b1 = new BcastMsg;
		b1->Broadcast(true);
		delete b1;

		if (s > 1)
		{
			PPongMsg m;
			m.Send(1);

			pthread_cond_wait(&w8, &lck);
			std::cerr << "lock signalled\n";
			pthread_mutex_unlock(&lck);
		}

		theApplication.QuitApplication();
	}

	theApplication.Wait();
}
