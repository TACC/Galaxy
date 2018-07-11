#pragma once

#include <iostream>
#include <memory>

namespace gxy
{
class smem
{
public:
  ~smem();

	static std::shared_ptr<smem> New(int n) { return std::shared_ptr<smem>(new smem(n)); }

	unsigned char *get()      { return ptr; }
	int   get_size() { return sz;  }

private:
  smem(int n);
	unsigned char *ptr;
	int sz;
	int kk;
};

typedef std::shared_ptr<smem> SharedP;
}