#include <ospray/ospray.h>
#include <SDK/common/Managed.h>

#if 0
void ospray::postStatusMsg(const std::string& s, unsigned int i)
{
	std::stringstream ss;
	ss << s;
	ospray::postStatusMsg(ss, i);
}
#endif

namespace osp_util
{
	void *
	GetIE(void *a)
	{
		ospray::ManagedObject *b = (ospray::ManagedObject *)a;
		return (void *)b->getIE();
	}
}
