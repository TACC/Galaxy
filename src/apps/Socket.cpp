// ========================================================================== //
// Copyright (c) 2014-2019 The University of Texas at Austin.                 //
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
#include <unistd.h>
#include <stdlib.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <string.h>
#include <pthread.h>

#include "Socket.h"

using namespace std;

Socket::Socket(int port)
{
	pthread_mutex_init(&lock, NULL);

	char hn[256];
	gethostname(hn, sizeof(hn));
	cerr << "Waiting at port " << port << " on host " << hn << endl;

  int tmp_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	unsigned int clilen;
  struct sockaddr_in serv_addr, cli_addr;

  bzero((char *) &serv_addr, sizeof(serv_addr));

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(port);

  if (::bind(tmp_sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
  {
    perror("ERROR on binding");
    exit(1);
  }

  listen(tmp_sockfd,5);

  clilen = sizeof(cli_addr);
  sockfd = accept(tmp_sockfd, (struct sockaddr *)&cli_addr, &clilen);

  if (sockfd < 0)
  {
    perror("ERROR on accept");
    exit(1);
  }

	int flag = 0;
	setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (char *) &flag, sizeof(int));

	int n = 1234;
	Send((char*)&n, sizeof(n));
	char *b;
	Recv(b, n);
	if (*(int *)b != 4321) 
	{
		cerr << "error connecting socket" << endl;
		exit(1);
	}
	cerr << "socket established" << endl;
}

Socket::Socket(char *host, int port)
{
	pthread_mutex_init(&lock, NULL);

  struct sockaddr_in serv_addr;
  struct hostent *server;

  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  
  if (sockfd < 0) 
	{
   perror("ERROR opening socket");
   exit(1);
  }
	
  server = gethostbyname(host);
  
  if (server == NULL) 
	{
   cerr << "ERROR no such host: " << host << endl;
   exit(1);
  }
  
  bzero((char *) &serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
  serv_addr.sin_port = htons(port);
  
  if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) 
	{
    perror("ERROR connecting");
    exit(1);
  }

	int flag = 1;
	setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (char *) &flag, sizeof(int));

	char *b; int n;
	Recv(b, n);
	if (*(int *)b != 1234) 
	{
		cerr << "error connecting socket" << endl;
		exit(1);
	}
	n = 4321;
	Send((char*)&n, sizeof(n));

	cerr << "socket established" << endl;
}

void
Socket::Send(char *b, int n)
{
  pthread_mutex_lock(&lock);

	write(sockfd, &n, sizeof(n));
	while(n)
	{

		int t = write(sockfd, b, n);
		n -= t;
		b += t;
	}

  pthread_mutex_unlock(&lock);
}

void
Socket::SendV(char **b, int* n)
{
  pthread_mutex_lock(&lock);

	int sz = 0;
	for (int i = 0; n[i]; i++)
		sz += n[i];

	write(sockfd, &sz, sizeof(sz));
	for (int i = 0; n[i]; i++)
	{
		int nn = n[i];
		char *bb = b[i];
		while(nn)
		{
			int t = write(sockfd, bb, nn);
			nn -= t;
			bb += t;
		}
	}

  pthread_mutex_unlock(&lock);
}

void
Socket::Recv(char*& b, int& n)
{
	fd_set fds;

	FD_ZERO(&fds);
  FD_SET(sockfd, &fds);
	if (select(sockfd+1, &fds, NULL, NULL, NULL) < 0)
	{
		perror("select()");
		exit(1);
	}
	
	read(sockfd, &n, sizeof(n));
	b = (char *)malloc(n);
	char *bb = b;
	int nn = n;
	while (nn)
	{
		int t = read(sockfd, bb, nn);
		bb += t;
		nn -= t;
	}
}
