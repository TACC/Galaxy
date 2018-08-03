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

/*! \file MessageManager.h 
 * \brief manages communication of interprocess Messages in Galaxy
 */

#include <iostream>
#include <memory>
#include <mpi.h>
#include <stdlib.h>

#include "Application.h"
#include "Message.h"
#include "MessageQ.h"

namespace gxy
{

class Application;

//! manages communication of interprocess Messages in Galaxy
/*! \ingroup framework
 * \sa Message, MessageQ, Work
 */
class MessageManager {
public:
  MessageManager(); //!< default constructor
  ~MessageManager(); //!< default destructor

  //! start the message manager
  /*! The MessageManager handles all Messages containing Work in Galaxy. Usually this will be done
   * over MPI among several distributed processes. However, even in a single-process case, a MessageManager
   * must still be created, but with `m = false`. `m` should also be `false` if Galaxy has a single render 
   * process that communicates to a remote GUI process via socket rather than MPI.
   * \param m will this MessageManager use MPI?
   */
  void Start(bool m = true);	// By default, servering an external connection
	void WaitForShutdown(); // TODO: unused?
	
	//! resume message processing after a Pause
	void Run();
	//! temporarily stop processing messages, resume with Run
	void Pause();

	//! get the size of the MPI pool associated with this MessageManager
  int GetSize() { return mpi_size; }
  //! get the rank of this process in the associated MPI pool
  int GetRank() { return mpi_rank; }

  //! set the size of the MPI pool associated with this MessageManager
  void SetSize(int s) { mpi_size = s; }
  //! set the rank of this process in the associated MPI pool
  void SetRank(int r) { mpi_rank = r; }

  //! set the MPI communicator for point-to-point communications
	void setP2PComm(MPI_Comm p) { p2p_comm = p; }
	//! set the MPI communicator for collective communications
	void setCollComm(MPI_Comm c) { coll_comm = c; }

	//! get the MPI communicator for point-to-point communications
	MPI_Comm getP2PComm() { return p2p_comm; }
	//! get the MPI communicator for collective communications
	MPI_Comm getCollComm() { return coll_comm; }

	//! returns a pointer to the incoming MessageQ for this manager
  MessageQ *GetIncomingMessageQueue() { return theIncomingQueue; }
  //! returns a pointer to the outgoing MessageQ for this manager
  MessageQ *GetOutgoingMessageQueue() { return theOutgoingQueue; }

  //! send a Work object to a remote process
  /*! creates a Message containing a serialization of the given Work object,
   * then sends the Message to destination process
   * \param w a pointer to the desired Work payload
   * \param dest the MPI rank of the destination process
   */
	void SendWork(Work *w, int dest);

  //! send a Work object to all remote process
  /*! creates a Message containing a serialization of the given Work object,
   * then sends the Message to destination process
   * \param w a pointer to the desired Work payload
   * \param collective should the Message be collective (i.e. synchronizing)?
   * \param blocking should this process block until the Message is sent?
   */
	void BroadcastWork(Work *w, bool collective, bool blocking);


	//! This method ships the message to the destination given in the message.
	void ExportDirect(Message *m, MPI_Request *, MPI_Request *, int); // TODO: unused?

	//! send an existing message according to its embedded type
	/*! This method handles either broadcast or direct messages; if the 
	 * former, it determines the destinations in the tree distribution
	 * pattern and calls ExportDirect to ship it to up to two other 
	 * destinations; otherwise calls ExportDirect to ship to the 
	 * message's p2p destination.   Returns number of messages sent:
	 * 1, in p2p, 0, 1 or 2 in bcast
	 */
	int Export(Message *m);				

	//! get the socket for a client-server connection
	int get_clientserver_skt() { return clientserver_skt; }
	//! set the socket for a client-server connection
	void set_clientserver_skt(int s) { clientserver_skt = s; }

	//! get the size of the next message to be sent on the client-server socket
	int get_next_message_size() { return next_message_size; }
	//! set the size of the next message to be sent on the client-server socket
	void set_next_message_size(int s) { next_message_size = s; }

	//! print the contents of the incoming and outgoing message queues
	void dump();

	void Lock()   { pthread_mutex_lock(&lock); } //!< lock the message management thread mutex
	void Unlock() { pthread_mutex_unlock(&lock); } //!< unlock the message management thread mutex
	void Signal() { pthread_cond_signal(&cond); } //!< signal the message management thread to resume after a Wait
	void Wait()   { pthread_cond_wait(&cond, &lock); } //!< pause the message management thread, resume with Signal

	//! is this message manager using MPI?
	bool UsingMPI() { return with_mpi; }

private:
	int clientserver_skt;
	int next_message_size;

	static void setup_mpi(Application*, MessageManager*);
	static bool check_clientserver(MessageManager*);
	static bool check_mpi(MessageManager*);
	static bool check_outgoing(MessageManager*);

  static void *messageThread(void *);
  static void *workThread(void *);

  MessageQ *theIncomingQueue;
  MessageQ *theOutgoingQueue;

  pthread_mutex_t lock;
  pthread_cond_t cond;

  pthread_t message_tid;
  pthread_t work_tid;

  int wait;
  int mpi_rank;
  int mpi_size;

	bool with_mpi;
	bool pause;
	bool quit;

	MPI_Comm p2p_comm, coll_comm;
};

} // namespace gxy

