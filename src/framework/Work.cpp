#include <iostream>
#include "Application.h"
#include "Work.h"
#include "MessageManager.h"

using namespace std;
namespace pvol
{

Work::Work()
{
}

Work::Work(int n)
{
	if (n > 0)
		contents = smem::New(n);
	else
		contents = nullptr;
}

Work::Work(SharedP sp)
{
	contents = sp;
}

Work::Work(const Work* o)
{
	contents = o->contents;
	className = o->className;
	type = o->type;
}

int
Work::RegisterSubclass(string name, Work *(*d)(SharedP))
{
	return GetTheApplication()->RegisterWork(name, d);
}

void 
Work::Send(int i)
{
	Application *app = GetTheApplication();
  MessageManager *mm = app->GetTheMessageManager();
	mm->SendWork(this, i);
}

void 
Work::Broadcast(bool c, bool b)
{
	Application *app = GetTheApplication();
  MessageManager *mm = app->GetTheMessageManager();
	mm->BroadcastWork(this, c, b);
}

Work::~Work()
{
}
}

