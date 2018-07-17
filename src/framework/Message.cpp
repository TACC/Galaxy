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

#include "Message.h"

#include <fstream>
#include <iostream>
#include <memory>
#include <mpi.h>
#include <pthread.h>
#include <sstream>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "Application.h"

using namespace std;

namespace gxy
{

Message::Message(Work *w, bool collective, bool blk)
{
  content = w->get_pointer();
  header.type = w->GetType();

  header.broadcast_root = GetTheApplication()->GetRank();
  header.sender = -1;

	header.collective = collective;

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
	header.collective = false;

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

	MPI_Status s0;
	if (count == sizeof(header))
	{
		MPI_Recv((unsigned char *)&header, count, MPI_UNSIGNED_CHAR, status.MPI_SOURCE, status.MPI_TAG, theMessageManager->getP2PComm(), &s0);
		content = nullptr;
	}
	else
	{
		unsigned char *buffer = (unsigned char *)malloc(count);
		MPI_Recv(buffer, count, MPI_UNSIGNED_CHAR, status.MPI_SOURCE, status.MPI_TAG, theMessageManager->getP2PComm(), &s0);
		memcpy(&header, buffer, sizeof(header));
		content = smem::New(header.content_size);
		memcpy(content->get(), buffer+sizeof(header), header.content_size);
		free(buffer);
	}
}

Message::Message(int skt, int size)
{
	if (recv(skt, &header, sizeof(header), 0) == -1)
	{
		cerr << "ERROR: error receiving Message header!" << endl;
		exit(1);
	}

	size -= sizeof(header);
	if (size)
	{
		content = smem::New(size);
		if (recv(skt, content->get(), size, 0) == -1)
		{
			cerr << "ERROR: error receiving body!" << endl;
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
    cerr << "WARNING: called Message::Wait for non-blocking message" << endl;
  else
    pthread_cond_wait(&cond, &lock);
}

} // namespace gxy
