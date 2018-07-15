// ========================================================================== //
// Copyright (c) 2014-2018 The University of Texas at Austin.                 //
// All rights reserved.                                                       //
//                                                                            //
// Licensed under the Apache License, Version 2.0 (the "License");            //
// you may not use this file except in compliance with the License.           //
// A copy of the License is included with this software in the file LICENSE.  //
// If your copy does not contain the License, you may obtain a copy of the    //
// License at:                                                                //
//                                                                            //
//     https://www.apache.org/licenses/LICENSE-2.0                            //
//                                                                            //
// Unless required by applicable law or agreed to in writing, software        //
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT  //
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.           //
// See the License for the specific language governing permissions and        //
// limitations under the License.                                             //
//                                                                            //
// ========================================================================== //

#include <stdlib.h>
#include <string>
#include <string.h>
#include <iostream>
#include <fstream>
#include <math.h>

using namespace std;


void 
syntax(char *a)
{
	cerr << "syntax: " << a << " [options]\n";
	cerr << "options:\n";
	cerr << "  -x n         counts along x (100)\n";
	cerr << "  -y n         counts along y (100)\n";
	cerr << "  -z n         counts along z (100)\n";
	cerr << "  -xyz n       counts along all three axes (100)\n";
	cerr << "  -f fname     output file name root (foo)\n";
	exit(1);
}

int
main(int argc, char **argv)
{
	string froot("foo");
	int nx = 100;
	int ny = 100;
	int nz = 100;

	for (int i = 1; i < argc; i++)
	{
		if (! strcmp(argv[i], "-x")) nx = atoi(argv[++i]);
		else if (! strcmp(argv[i], "-y")) ny = atoi(argv[++i]);
		else if (! strcmp(argv[i], "-z")) nz = atoi(argv[++i]);
		else if (! strcmp(argv[i], "-xyz")) { nx = atoi(argv[++i]); ny = nx; nz = nx; }
		else if (! strcmp(argv[i], "-f")) froot = argv[++i];
		else syntax(argv[0]);
	}
		
	float buf[nx*ny*nz];
	float *b = buf;
	for (int i = 0; i < nx; i++)
	{
		float x = -1 + (((float)i)/(nx-1))*2.0;
		for (int j = 0; j < ny; j++)
		{
			float y = -1 + (((float)j)/(ny-1))*2.0;
			for (int k = 0; k < nz; k++)
			{
				float z = -1 + (((float)k)/(nz-1))*2.0;
				// *b++ = sqrt(x*x + y*y + z*z) / sqrt(3.0);
				*b++ = sqrt(x*x + y*y) / sqrt(2.0);
			}
		}
	}
	

	ofstream out;
	out.open((froot + ".vol").c_str());
	out << "float\n";
	out << "-1 -1 -1\n";
	out << nx << " " << ny << " " << nz << "\n";
	out << (2.0 / (nx-1)) << " " << (2.0 / (ny-1)) << " " << (2.0 / (nz-1)) << "\n";
	out << "foo.raw\n";
	out.close();

	out.open((froot + ".raw").c_str());
	out.write((char *)buf, sizeof(buf));
	out.close();
}
