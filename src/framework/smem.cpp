#include <iostream>
#include "Application.h"
#include "smem.h"

using namespace std;

static int dbg = -2;
static int smbrk = -1;
static int k = 0;

namespace pvol
{
void
smem_catch(){}

smem::~smem()
{
	if (dbg == 1)
	{
		APP_LOG(<< "smem dtor " << kk << " " << sz << " at " << std::hex << ((long)ptr));
	}

	if (kk == smbrk)
		smem_catch();

	if (ptr) 
	{
		free(ptr);
	}
}

static int nalloc = 0;

smem::smem(int n)
{
	kk = k++;

	if (dbg == -2)
	{
		if (getenv("SMEMDBG"))
		{
			int d = atoi(getenv("SMEMDBG"));
			dbg = (d == -1 || d == GetTheApplication()->GetRank()) ? 1 : 0;
		}
		if (getenv("SMEMBRK"))
			smbrk = atoi(getenv("SMEMBRK"));
	}

	if (kk == smbrk)
		smem_catch();

	if (n > 0)
		ptr = (unsigned char *)malloc(n);
	else
		ptr = NULL;

	sz = n;

	if (dbg == 1)
	{
		 APP_LOG(<< "smem ctor " << kk << " " << sz << " at " << std::hex << ((long)ptr));
	}
}
}