#undef DBG

#include <mpi.h>
#include <pthread.h>
#include <unistd.h>
#include <iostream>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "Application.h"
#include "MessageManager.h"

#include "Message.h"
#include "MessageQ.h"

#include <string>
#include <fstream>
#include <sstream>
#include <memory>

#define LOGGING 0

void
killer(){}

bool show_message_arrival;

struct mpi_send_buffer
{
	// If p2p, use left

	MPI_Request lrq;	
	MPI_Request rrq; 

	int n;
	Message *msg;
	unsigned char *send_buffer;
	int total_size;
};

vector<mpi_send_buffer *> mpi_in_flight;

void
purge_completed_mpi_buffers()
{
	bool done = false;
	while (! done)
	{
		done = true;
		for (vector<mpi_send_buffer *>::iterator i = mpi_in_flight.begin(); done && i != mpi_in_flight.end(); i++)
		{
			int lflag, rflag; MPI_Status s;
			mpi_send_buffer *m = (*i);

			MPI_Test(&m->lrq, &lflag, &s);

			if (m->n > 1)
				MPI_Test(&m->rrq, &rflag, &s);
			else
				rflag = true;

			if (lflag && rflag) // if left is gone or, if bcast, BOTH are gone, then 
			{
				done = false;

				mpi_in_flight.erase(i);

				if (m->msg->GetHeader() != m->send_buffer)
					free(m->send_buffer);

				int r = GetTheApplication()->GetRank();
				if (m->msg->isBlocking() && (r == m->msg->GetSender() || r == m->msg->GetRoot()))
					m->msg->Signal();
				else
					delete m->msg;
			
				delete m;
			}
		}
	}
}

void
purge_all_mpi_buffers()
{
	while (mpi_in_flight.size() > 0)
		purge_completed_mpi_buffers();
}

void *
MessageManager::workThread(void *p)
{
  MessageManager *mm = (MessageManager *)p;
  Application *app = GetTheApplication();

	register_thread("workThread");

  mm->Lock();
  mm->wait--;
	if (mm->wait == 0)
		mm->Wait();
  mm->Unlock();

  Message m;
  while (!mm->quit)
	{
		Message *m = mm->GetIncomingMessageQueue()->Dequeue();

		if (! app->Running())
			break;

    Work *w = app->Deserialize(m);

    w->Action(m->GetSender());
    delete w;
		delete m;
  }

  pthread_exit(NULL);
}

void
MessageManager::Pause()
{
#if LOGGING
	APP_LOG(<< "MessageManager::Pause\n");
#endif
  pause = true;
  Lock();
}

void
MessageManager::Run()
{
#if LOGGING
	APP_LOG(<< "MessageManager::Run\n");
#endif
  pause = false;
	Signal();
	Unlock();
}

void *MessageManager::messageThread(void *p)
{
  Application *app = GetTheApplication();
  MessageManager *mm = app->GetTheMessageManager();

  if (pthread_create(&mm->work_tid, NULL, workThread, mm))
	{
    std::cerr << "Failed to spawn work thread\n";
    exit(1);
  }

	register_thread("messageThread");

	bool with_mpi = (bool)p;
	setup_mpi(app, mm, with_mpi);

	mm->Lock();
  mm->wait--;
	if (mm->wait == 0)
		mm->Signal();
  mm->Unlock();

	double lastTime = GetTheEventTracker()->gettime();

  while (!mm->quit)
	{
		int read_ready; MPI_Status status;

		if (mm->pause)
		{
			mm->Lock();
			while (mm->pause && !mm->quit)
				mm->Wait();
			mm->Unlock();
		}

		if (! mm->quit)
		{
			// Lets see if there's a client/server message
		
			mm->quit = check_clientserver(mm);

			// Anything coming across MPI?
	
			if (! mm->quit)
         mm->quit = check_mpi(mm);

			// Anything outgoing?

			if (! mm->quit)
        mm->quit = check_outgoing(mm);

			purge_completed_mpi_buffers();

			double thisTime = GetTheEventTracker()->gettime();
			if (thisTime - lastTime > 1.0)
			{
				stringstream xx;
				xx << "message loop took " << (thisTime - lastTime) << " seconds";
				APP_PRINT(<< xx.str());
			}
			lastTime = thisTime;
		}

		if (mm->quit) app->Kill();
	}

	purge_all_mpi_buffers();

	mm->GetOutgoingMessageQueue()->Kill();
	mm->GetIncomingMessageQueue()->Kill();

	mm->Lock();
	mm->Signal();
	if (mm->work_tid) pthread_join(mm->work_tid, NULL);
	mm->Unlock();

	MPI_Finalize();

  pthread_exit(NULL);
}

MessageManager::MessageManager()
{
	clientserver_skt = -1;
	message_tid = 0;
	work_tid = 0;

  pthread_mutex_init(&lock, NULL);
  pthread_cond_init(&cond, NULL);

  Lock();

  theIncomingQueue = new MessageQ("incoming");
  theOutgoingQueue = new MessageQ("outgoing");

	// Message *m = new Message;
	// GetOutgoingMessageQueue()->Enqueue(m);

	pause = true;
	quit  = false;

  Unlock();
}

MessageManager::~MessageManager()
{
	if (message_tid) pthread_join(message_tid, NULL);

	delete theIncomingQueue;
	delete theOutgoingQueue;

	Unlock();
}

void MessageManager::Start(bool with_mpi)
{
	wait = 2;

  if (pthread_create(&message_tid, NULL, messageThread, (void *)with_mpi))
	{
    std::cerr << "Failed to spawn messaging thread\n";
    exit(1);
  }

  while (wait)
		Wait();
}

void
MessageManager::SendWork(Work *w, int dest)
{
	Message* m = new Message(w, dest);
	if (dest == GetTheApplication()->GetRank())
		GetIncomingMessageQueue()->Enqueue(m);
	else
		GetOutgoingMessageQueue()->Enqueue(m);
}

void
MessageManager::BroadcastWork(Work *w, bool block)
{
	Message* m = new Message(w, block);
	GetOutgoingMessageQueue()->Enqueue(m);

	if (block)
	{
		m->Wait();
		delete m;
	}
}

int
MessageManager::Export(Message *m)
{
	static int t = 0;
	int k = 0;
  int rank = GetTheApplication()->GetRank();
	int size = GetTheApplication()->GetSize();
	int root = m->GetRoot();

	// My rank relative to the broadcast root
	
	int d = ((size + rank) - m->GetRoot()) % size;

	// Only export if its either P2P or broadcast and this isn't a leaf

	if (m->IsBroadcast() && (2*d + 1) >= size)
		return 0;

	int tag = (MPI_TAG_UB) ? t % MPI_TAG_UB : t % 65535;
	t++;

	// If its a broadcast message, choose up to two destinations based
	// on the broadcast root, the rank and the size.  Otherwise, just ship it.

	struct mpi_send_buffer *msb = new mpi_send_buffer;
	msb->msg = m;

	// Send_buffer is the pointer that'll actually get written.
	// If the message has content, allocate and copy in.  Otherwise
	// just point at the header address
	
  if (m->HasContent())
	{
		msb->total_size = m->GetHeaderSize() + m->GetSize();
		msb->send_buffer = (unsigned char *)malloc(msb->total_size);
		memcpy(msb->send_buffer, m->GetHeader(), m->GetHeaderSize());
		memcpy(msb->send_buffer + m->GetHeaderSize(), m->GetContent(), m->GetSize());
	}
	else
	{
		msb->send_buffer = m->GetHeader();
		msb->total_size = m->GetHeaderSize();
	}

  if (m->IsBroadcast())
	{
    int l = (2 * d) + 1;
		int destination = (root + l) % size;
		MPI_Isend(msb->send_buffer, msb->total_size, MPI_UNSIGNED_CHAR, destination, tag, p2p_comm, &msb->lrq);
		k++;

		if ((l + 1) < size)
		{
			destination = (root + l + 1) % size;
			MPI_Isend(msb->send_buffer, msb->total_size, MPI_UNSIGNED_CHAR, destination, tag, p2p_comm, &msb->rrq);
			k++;
		}
  }
	else
	{
		MPI_Isend(msb->send_buffer, msb->total_size, MPI_UNSIGNED_CHAR, m->GetDestination(), tag, p2p_comm, &msb->lrq);
		k++;
	}

	msb->n = k;
	mpi_in_flight.push_back(msb);

	return k;
}

bool 
MessageManager::check_mpi(MessageManager *mm)
{
	Application *app = GetTheApplication();
	
	bool kill_app = false;
	int  read_ready;
	MPI_Status status;

	MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, mm->getP2PComm(), &read_ready, &status);

	if (read_ready)
	{
		double t0 = GetTheEventTracker()->gettime();

		Message *incoming_message = new Message(status);

		double t1 = GetTheEventTracker()->gettime();

		char buf[1024];
		strcpy(buf, app->Identify(incoming_message));

		if (incoming_message->IsBroadcast())
		{
			mm->Export(incoming_message);

			Work *w  = app->Deserialize(incoming_message);
			double t2 = GetTheEventTracker()->gettime();
			kill_app = w->CollectiveAction(mm->getCollComm(), GetTheApplication()->GetRank() == incoming_message->header.broadcast_root);
			if (kill_app) killer(); // for debugging
			delete w;

			double t3 = GetTheEventTracker()->gettime();
			if ((t3 - t0) > 1.0)
			{
				stringstream xx;
				xx << "bcast took " << (t3 - t0) << " seconds to handle MPI request " << buf << ": ";
				if ((t1 - t0) > 1.0)
					xx << (t1 - t0) << " seconds for New ";
				if ((t2 - t1) > 1.0)
					xx << (t2 - t1) << " seconds for Deserialize ";
				if ((t3 - t2) > 1.0)
					xx << (t3 - t2) << " seconds for CollectiveAction";
				APP_PRINT(<< xx.str());
			}
		}
		else
		{
			double t0 = GetTheEventTracker()->gettime();
			mm->GetIncomingMessageQueue()->Enqueue(incoming_message);
			double t1 = GetTheEventTracker()->gettime();
			if ((t1 - t0) > 1.0)
			{
				stringstream xx;
				xx << "p2p took " << (t1 - t0) << " seconds to handle ENQUEUE MPI request (" << buf << ")";
				APP_PRINT(<< xx.str());
			}
		}
	}


	return kill_app;
}

bool
MessageManager::check_outgoing(MessageManager *mm)
{
	Application *app = GetTheApplication();
	bool kill_app = false;

	if (mm->GetOutgoingMessageQueue()->IsReady())
	{
		Message *omsg = mm->GetOutgoingMessageQueue()->Dequeue();
		
		int nsent = 0;
		if (omsg)
		{
			if (omsg->IsBroadcast() || (omsg->GetDestination() != app->GetRank()))
				nsent = mm->Export(omsg);

			if (omsg->IsBroadcast())
			{
				Work *w = app->Deserialize(omsg);
				kill_app = w->CollectiveAction(mm->getCollComm(), omsg->header.broadcast_root == GetTheApplication()->GetRank());
				if (kill_app) killer(); // for debugging
				delete w;
			}

			// If nsent > 0 we will do the signal or delete the message
			// when we know the buffered messages have been sent.  Otherwise, 
			// we do it here.

			if (nsent == 0) 
			{
				if (omsg->isBlocking())
					omsg->Signal();        // blocked guy will delete
				else 
					delete omsg;
			}
		}
	}

	return kill_app;
}
		
bool
MessageManager::check_clientserver(MessageManager *mm)
{
	Application *app = GetTheApplication();
	int skt = mm->get_clientserver_skt();
	if (skt > 0)
	{
		int bytes_available;
		ioctl(skt, FIONREAD, &bytes_available);

		if (bytes_available)
		{
			int nms = mm->get_next_message_size();

			if (nms == -1)
			{
				if (recv(skt, &nms, sizeof(nms), 0) == -1)
				{
					std::cerr << "error receiving socket message from other guy\n";
					exit(1);
				}

				bytes_available -= nms;
			}

			if (bytes_available >= nms)
			{
				Message *incoming_message = new Message(skt, nms);
				nms = -1;

				mm->GetIncomingMessageQueue()->Enqueue(incoming_message);
			}

			mm->set_next_message_size(nms);
		}
	}
	return false;
}

void
MessageManager::setup_mpi(Application *app, MessageManager *mm, bool use_mpi)
{
	if (use_mpi)
	{
		int pvd;
#if 0
		MPI_Init_thread(app->GetPArgC(), app->GetPArgV(), MPI_THREAD_MULTIPLE, &pvd);

		if (pvd != MPI_THREAD_MULTIPLE)
		{
			std::cerr << "Error ... MPI threading\n";
			exit(1);
		}
#else
		MPI_Init(app->GetPArgC(), (char ***)app->GetPArgV());
#endif

		MPI_Comm p2p, coll;
		MPI_Comm_dup(MPI_COMM_WORLD, &p2p);
		MPI_Comm_dup(MPI_COMM_WORLD, &coll);

		mm->setP2PComm(p2p);
		mm->setCollComm(coll);

		int rs, sz;
		MPI_Comm_rank(MPI_COMM_WORLD, &rs);
		mm->SetRank(rs);
		MPI_Comm_size(MPI_COMM_WORLD, &sz);
		mm->SetSize(sz);

#if defined(EVENT_TRACKING)

    class PIDEvent : public Event
    {
    public:
      PIDEvent() 
      {
        p = (long)getpid();
      }

    protected:
      void print(ostream& o)
      {
        Event::print(o);
        o << "MPI process ID: " << p;
      }

    private:
      long p;
    };

  GetTheEventTracker()->Add(new PIDEvent());
#endif

	}
	else
	{
		mm->SetRank(0);
		mm->SetSize(1);
	}
}

void 
MessageManager::dump()
{
	GetIncomingMessageQueue()->printContents();
	GetOutgoingMessageQueue()->printContents();
}

