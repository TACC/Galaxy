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

Socket::Socket(int port)
{
	lock = PTHREAD_MUTEX_INITIALIZER;

	char hn[256];
	gethostname(hn, sizeof(hn));
	std::cerr << "waiting on host " << hn << " port " << port << "\n";

  int tmp_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	unsigned int clilen;
  struct sockaddr_in serv_addr, cli_addr;

  bzero((char *) &serv_addr, sizeof(serv_addr));

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(port);

  if (bind(tmp_sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
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
		std::cerr << "error connecting socket\n";
		exit(1);
	}
	std::cerr << " socket established\n";
}

Socket::Socket(char *host, int port)
{
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
   fprintf(stderr,"ERROR, no such host\n");
   exit(0);
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
		std::cerr << "error connecting socket\n";
		exit(1);
	}
	n = 4321;
	Send((char*)&n, sizeof(n));

	std::cerr << *(int *)b << " socket established\n";
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
