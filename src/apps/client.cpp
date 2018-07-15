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

#include <unistd.h>
#include <stdio.h>
#include <iostream>
#include "ClientServer.h"

using namespace pvol;

int
main(int argc, char **argv)
{
	ClientServer cs;

	if (argc == 1)
		cs.setup_client("localhost");
	else
		cs.setup_client(argv[1]);
		
	std::cerr << "setup OK\n";

	char c;
	do
	{
		c = getchar();
		std::cerr << "got " << c << "\n";
		if (c == 's')
			write(cs.get_socket(), &c, 1);
	}  while (c != 'q');
}

