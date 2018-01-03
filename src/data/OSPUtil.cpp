#include <ospray/ospray.h>
#include <SDK/api/Device.h>

namespace osp_util
{
	void *
	GetIE(void *a)
	{
		ospray::ManagedObject *b = (ospray::ManagedObject *)a;
		return (void *)b->getIE();
	}
}
