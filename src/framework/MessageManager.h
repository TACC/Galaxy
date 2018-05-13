#pragma once

#include <mpi.h>
#include <stdlib.h>
#include <iostream>
#include <memory>
#include "Application.h"
#include "Message.h"
#include "MessageQ.h"

namespace pvol
{
class Application;

class MessageManager {
public:
  MessageManager();
  ~MessageManager();

  void Start(bool with_mpi = true);	// By default, not servering an external connection
	void WaitForShutdown();
	
	void Run();
	void Pause();

  int GetSize() { return mpi_size; }
  int GetRank() { return mpi_rank; }

  void SetSize(int s) { mpi_size = s; }
  void SetRank(int r) { mpi_rank = r; }

	void setP2PComm(MPI_Comm p) { p2p_comm = p; }
	void setCollComm(MPI_Comm c) { coll_comm = c; }

	MPI_Comm getP2PComm() { return p2p_comm; }
	MPI_Comm getCollComm() { return coll_comm; }

  MessageQ *GetIncomingMessageQueue() { return theIncomingQueue; }
  MessageQ *GetOutgoingMessageQueue() { return theOutgoingQueue; }

	void SendWork(Work *w, int dest);
	void BroadcastWork(Work *w, bool collective, bool blocking);


	// This method ships the message to the destination given in the
	// message.

	void ExportDirect(Message *m, MPI_Request *, MPI_Request *, int);

	// This method handles either broadcast or direct messages; if the 
	// former, it determines the destinations in the tree distribution
	// pattern and calls ExportDirect to ship it to up to two other 
	// destinations; otherwise calls ExportDirect to ship to the 
	// message's p2p destination.   Returns number of messages sent:
	// 1, in p2p, 0, 1 or 2 in bcast

	int Export(Message *m);				

	int get_clientserver_skt() { return clientserver_skt; }
	void set_clientserver_skt(int s) { clientserver_skt = s; }

	int get_next_message_size() { return next_message_size; }
	void set_next_message_size(int s) { next_message_size = s; }

	void dump();

	void Lock()   { pthread_mutex_lock(&lock); }
	void Unlock() { pthread_mutex_unlock(&lock); }
	void Signal() { pthread_cond_signal(&cond); }
	void Wait()   { pthread_cond_wait(&cond, &lock); }

private:
	int clientserver_skt;
	int next_message_size;

	static void setup_mpi(Application*, MessageManager*, bool with_mpi);
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

	bool pause;
	bool quit;

	MPI_Comm p2p_comm, coll_comm;
};
}

