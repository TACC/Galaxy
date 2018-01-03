#undef DBG

#include <mpi.h>
#include <pthread.h>
#include <unistd.h>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include "Application.h"
#include "Message.h"
#include <string>
#include <fstream>
#include <sstream>
#include <memory>

#define LOGGING 0

Message::Message(Work *w, bool blk)
{
  content = w->get_pointer();
  header.type = w->GetType();

  header.broadcast_root = GetTheApplication()->GetRank();
  header.sender = -1;

  if (content != nullptr)
		header.content_size = content->get_size();
	else
		header.content_size = 0;

  blocking = blk;
  if (blocking)
	{
    pthread_mutex_init(&lock, NULL);
    pthread_cond_init(&cond, NULL);
    pthread_mutex_lock(&lock);
  }
}

Message::Message(Work *w, int i)
{
  blocking = false;

  content = w->get_pointer();

  header.type = w->GetType();

  header.sender = GetTheApplication()->GetRank();
  header.broadcast_root = -1;
  header.content_size = content->get_size();

  destination = i;
}

Message::Message(MPI_Status &status)
{
  Application *theApplication = GetTheApplication();
  MessageManager *theMessageManager = theApplication->GetTheMessageManager();

	int count;
	MPI_Get_count(&status, MPI_UNSIGNED_CHAR, &count);

	if (count != sizeof(header))
	  std::cerr << "ERROR 1\n";

	MPI_Status s0;
	MPI_Recv((unsigned char *)&header, sizeof(header), MPI_UNSIGNED_CHAR, status.MPI_SOURCE, status.MPI_TAG, theMessageManager->getP2PComm(), &s0);

	if (header.HasContent())
	{
		content = smem::New(header.content_size);
		MPI_Recv(content->get(), content->get_size(), MPI_UNSIGNED_CHAR, status.MPI_SOURCE, header.content_tag, theMessageManager->getP2PComm(), &s0);
#ifdef DBG
    APP_LOG(<< "message size: " << content->get_size() << " tag " << status.MPI_TAG << "/" << header.content_tag << " from: " << status.MPI_SOURCE);
#endif
	}
  else
	{
		content = nullptr;
#ifdef DBG
    APP_LOG(<< "header only tag " << status.MPI_TAG << " from: " << s0.MPI_SOURCE);
#endif
	}
}

Message::Message(int skt, int size)
{
	if (recv(skt, &header, sizeof(header), 0) == -1)
	{
		std::cerr << "error receiving header!\n";
		exit(1);
	}

	size -= sizeof(header);
	if (size)
	{
		content = smem::New(size);
		if (recv(skt, content->get(), size, 0) == -1)
		{
			std::cerr << "error receiving body!!\n";
			exit(1);
		}
	}
}

Message::Message()
{
}

Message::~Message()
{
}

void Message::Wait()
{
  if (!blocking)
    std::cerr << "Error... waiting on non-blocking message?\n";
  else
    pthread_cond_wait(&cond, &lock);
}
