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

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "ClientServer.h"

namespace gxy
{
ClientServer::ClientServer()
{
}

void
ClientServer::setup_server(int port)
{
	int      	sd;
	socklen_t addrlen;
	struct   	sockaddr_in sin;
	struct   	sockaddr_in pin;

	if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
			perror("socket");
			exit(1);
	}

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons(port);

	if (bind(sd, (struct sockaddr *) &sin, sizeof(sin)) == -1)
	{
			perror("bind");
			exit(1);
	}

	if (listen(sd, 1) == -1)
	{
			perror("listen");
			exit(1);
	}

	std::cerr << "waiting for connection...";

	/* wait for a client to talk to us */
	addrlen = sizeof(pin);
	if ((skt = accept(sd, (struct sockaddr *)  &pin, &addrlen)) == -1)
	{
			perror("accept");
			exit(1);
	}

	std::cerr << " connected\n";
}

void
ClientServer::setup_client(char *host, int port)
{
	struct sockaddr_in sin;
	struct sockaddr_in pin;
	struct hostent *hp;

	if ((hp = gethostbyname(host)) == 0)
	{
			perror("gethostbyname");
			exit(1);
	}

	memset(&pin, 0, sizeof(pin));
	pin.sin_family = AF_INET;
	pin.sin_addr.s_addr = ((struct in_addr *)(hp->h_addr))->s_addr;
	pin.sin_port = htons(port);

	if ((skt = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror("socket");
		exit(1);
	}

	std::cerr << "reaching out...";
	if (connect(skt,(struct sockaddr *) &pin, sizeof(pin)) == -1)
	{
		perror("connect");
		exit(1);
	}

	std::cerr << " connected\n";
}
}
