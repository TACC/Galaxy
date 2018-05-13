#pragma once

#include <iostream>
#include <memory>

using namespace std;

namespace pvol
{
class smem
{
public:
  ~smem();

	static shared_ptr<smem> New(int n) { return shared_ptr<smem>(new smem(n)); }

	unsigned char *get()      { return ptr; }
	int   get_size() { return sz;  }

private:
  smem(int n);
	unsigned char *ptr;
	int sz;
	int kk;
};

typedef shared_ptr<smem> SharedP;
}

