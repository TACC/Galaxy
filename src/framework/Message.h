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

#pragma once

#include <mpi.h>
#include <stdlib.h>
#include <iostream>
#include <memory>
#include "Work.h"

namespace gxy
{

class MessageManager;

class Message {
  friend class MessageManager;

public:
  // Point to point, fire-and-forget.   If i is -1, then we assume
  // its client/server and the destination is the other guy
 
  Message(Work *w, int i = -1);

  // Collective, one-to-all, will be run in MPI thread. May be blocking
	// for the caller

  Message(Work *w, bool blk = false);

	// Message to be read off MPI
	Message(MPI_Status&);

	// Message to be read off socket
	Message(int skt, int size);

  Message();

  ~Message();

	bool isBlocking() { return blocking; }
	void Signal()
	{ 
		pthread_mutex_lock(&lock);
		pthread_cond_signal(&cond);
		pthread_mutex_unlock(&lock);
	}



public:
	void Send(int i);
	void Broadcast();

  int  GetType() { return header.type; }
  void SetType(int t) { header.type = t; }

  unsigned char *GetContent() { return content->get(); }
  size_t GetSize() { return content->get_size(); }

  int GetRoot() { return header.broadcast_root; }
  int GetSender() { return header.sender; }

	bool IsBroadcast() { return header.broadcast_root != -1; }
	bool IsP2P() { return ! IsBroadcast(); }

	bool HasContent() { return header.HasContent(); }

  void SetDestination(int i) { destination = i; }
  int GetDestination() { return destination; }

  // Wait for a blocking message to be handled.  This will return
  // when the message has been sent, and if the message is a broadcast
  // message (that is, runs in the messaging thread), for the message's
  // action to happen locally.  ONLY VALID IF BLOCKING
  void Wait();

	SharedP ShareContent() { return content; }


protected:
  struct MessageHeader {

    int  broadcast_root; // will be -1 for point-to-point
		int  sender; 				 // will be -1 for broadcast
    int  type;
    int  content_tag;
    int  content_size;

		bool HasContent() { return content_size > 0; }

  } header;

	unsigned char *GetHeader() { return (unsigned char *)&header; }
	int   GetHeaderSize() { return sizeof(header); }

  int id;

  bool blocking;
  pthread_mutex_t lock;
  pthread_cond_t cond;

  SharedP content;
	int destination;

  MPI_Status status;
  MPI_Request request;
};
}
