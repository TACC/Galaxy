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

namespace gxy
{
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
}