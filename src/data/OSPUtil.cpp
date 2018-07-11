#include <ospray/ospray.h>
#include <ospray/SDK/common/Managed.h>
#include <ospray/SDK/api/Device.h>

namespace gxy
{
namespace osp_util
{
	void *
	GetIE(void *a)
	{
		ospray::ManagedObject *b = (ospray::ManagedObject *)a;
		return (void *)b->getIE();
	}
}
}