// ========================================================================== //
// Copyright (c) 2014-2020 The University of Texas at Austin.                 //
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

/*! \file Message.h 
 * \brief represents an interprocess Work message in Galaxy
 * \ingroup framework
 */

#include <mpi.h>
#include <stdlib.h>
#include <iostream>
#include <memory>
#include "Work.h"

namespace gxy
{

class MessageManager;

//! a unit of interprocess communication in Galaxy
/*! \ingroup framework
 * \sa MessageManager, Work
 */
class Message {
  friend class MessageManager;

public:
  //! constructor for a point-to-point message
  /*! A point-to-point, fire-and-forget (non-blocking) message.   If i is -1, then we assume
   * a client/server configuration and the destination is the other guy.
   * \param w the Work message payload
   * \param i the destination process (-1 for client-server special case)
   */
  Message(Work *w, int i = -1);

  //! constructor for a collective (broadcast, one-to-all) message
  /*! A one-to-all message, which, if collective, will be run in Galaxy's MPI communication thread.
	 * The message send can be set to be blocking for the sender.
   * \param w the Work message payload
   * \param collective is this Message collective (i.e. a synchronization point across processes)?
   * \param blk should the sender block on this message until sent?
   */
  Message(Work *w, bool collective, bool blk);

	//! constructor for a Message to be read off an MPI communicator
	Message(MPI_Status&);

	//! constructor for a Message to be read off a socket
  /*! \param skt the socket where the message is to be read
   * \param size the expected size of the message
   */
	Message(int skt, int size);

  Message(const Message& o); //! copy constructor

  Message(); //!< default constructor

  ~Message(); //!< default destructor

  //! is this message blocking?
	bool isBlocking() { return blocking; }

  //! signal to a blocking thread that it should resume processing
	void Signal()
	{ 
		pthread_mutex_lock(&lock);
		busy = false;
		pthread_cond_signal(&cond);
		pthread_mutex_unlock(&lock);
	}

  //! is this thread 
	bool IsBusy() { return busy; }

  //! return message type (Work type dependent)
  int  GetType() { return header.type; }
  //! set message type (Work type dependent)
  void SetType(int t) { header.type = t; }

  //! get the serialized content (payload) of the message as a byte stream
  unsigned char *GetContent() { return content->get(); }
  //! get the size in bytes of the serialized message contents (payload)
  size_t GetSize() { return content->get_size(); }

  //! get process id for the broadcast root, -1 for point-to-point message
  int GetRoot() { return header.broadcast_root; }
  //! get the process id for the point-to-point message sender, -1 for broadcast message
  int GetSender() { return header.sender; }

  //! is this message a broadcast message?
	bool IsBroadcast() { return header.broadcast_root != -1; }
	//! is this message a point-to-point message?
  bool IsP2P() { return ! IsBroadcast(); }

  //! does this message have any (non-zero) content (payload)?
	bool HasContent() { return header.HasContent(); }

  //! set the destination process id for a point-to-point message (ignored for broadcast)
  void SetDestination(int i) { destination = i; }
  //! get the destination process id for a point-to-point message (ignored for broadcast)
  int GetDestination() { return destination; }

  //! return the serialized byte stream header for this message
	unsigned char *GetHeader() { return (unsigned char *)&header; }
  //! return the size of the serialized byte stream header for this message
	int   GetHeaderSize() { return sizeof(header); }

  //! Wait for a blocking message to be handled.  
  /*! This will return when the message has been sent, and if the message is a broadcast
   * message (that is, runs in the messaging thread), for the message's
   * action to happen locally.  
   * \warning only valid for blocking messages
   */
  void Wait();

  //! return a shared pointer to this message's content (payload)
	SharedP ShareContent() { return content; }

  //! is this message collective (i.e. synchrnoizing across processes)?
	bool IsCollective() { return header.collective; }

protected:
  struct MessageHeader {

    int  broadcast_root; // will be -1 for point-to-point
		int  sender; 				 // will be -1 for broadcast
    int  type;
    bool collective;
    int  content_size;

		bool HasContent() { return content_size > 0; }

  } header;

  int id;

  bool blocking;

  pthread_mutex_t lock;
  pthread_cond_t cond;
	
	bool busy;

  SharedP content;
	int destination;

  MPI_Status status;
  MPI_Request request;
};

} // namespace gxy

