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

using namespace std;

#define LOGGING 0

namespace gxy
{
void
killer(){}

bool show_message_arrival;

struct mpi_send_buffer
{
	// If p2p, use left

	MPI_Request lhrq, rhrq;	// left and right header request
	MPI_Request lbrq, rbrq;  // left and right body request
	int n;
	Message *msg;
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

			if (m->msg->HasContent())
				MPI_Test(&m->lbrq, &lflag, &s);		// Assume that if the body's gone the header must be gone
			else
				MPI_Test(&m->lhrq, &lflag, &s);	

			if (m->n > 1)
			{
				if (m->msg->HasContent())
					MPI_Test(&m->rbrq, &rflag, &s);		// Assume that if the body's gone the header must be gone
				else
					MPI_Test(&m->rhrq, &rflag, &s);	
			}
			else
				rflag = true;

			if (lflag && rflag) // if left is gone or, if bcast, BOTH are gone, then 
			{
				done = false;
				mpi_in_flight.erase(i);

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

  while (!mm->quit)
	{
		int read_ready; MPI_Status status;

		if (mm->pause)
		{
			mm->Lock();
			while (mm->pause && !mm->quit)
				mm->Wait();
			mm->Unlock();

			MPI_Barrier(mm->getP2PComm());
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

void
MessageManager::ExportDirect(Message *m, MPI_Request *hrq, MPI_Request *brq, int tag)
{
  if (m->HasContent())
  {
    m->header.content_tag = tag;
    int err1 = MPI_Isend(m->GetHeader(), m->GetHeaderSize(), MPI_UNSIGNED_CHAR, m->GetDestination(), tag, p2p_comm, hrq);
		int err2 = MPI_Isend(m->GetContent(), m->GetSize(), MPI_UNSIGNED_CHAR, m->GetDestination(), m->header.content_tag, p2p_comm, brq);
#ifdef DBG
    APP_LOG(<< "message size " << m->GetSize() << " tag: " << tag << "/" << m->header.content_tag << " sent to " << m->GetDestination());
#endif
  }
  else
  {
    m->header.content_tag = -1;
    int err1 = MPI_Isend(m->GetHeader(), m->GetHeaderSize(), MPI_UNSIGNED_CHAR, m->GetDestination(), tag, p2p_comm, hrq);
#ifdef DBG
    APP_LOG(<< "header-only message tag: " << tag << " sent to " << m->GetDestination());
#endif
  }
}

int
MessageManager::Export(Message *m)
{
	static int t = 0;
	int k = 0;
  int rank = GetTheApplication()->GetRank();

	int tag = (MPI_TAG_UB) ? t % MPI_TAG_UB : t % 65535;
	t++;

	// If its a broadcast message, choose up to two destinations based
	// on the broadcast root, the rank and the size.  Otherwise, just ship it.

	struct mpi_send_buffer *msb = new mpi_send_buffer;
	msb->msg = m;

  if (m->IsBroadcast())
	{
		int size = GetTheApplication()->GetSize();
		int root = m->GetRoot();

    int d = ((size + rank) - m->GetRoot()) % size;

    int l = (2 * d) + 1;
    if (l < size)
		{
			m->SetDestination((root + l) % size);
			ExportDirect(m, &msb->lhrq, &msb->lbrq, tag);
			k++;

      if ((l + 1) < size)
			{
#if LOGGING
				APP_LOG(<< "Export broadcast: left and right");
#endif
				m->SetDestination((root + l + 1) % size);
				ExportDirect(m, &msb->rhrq, &msb->rbrq, tag);
				k++;
			}
#if LOGGING
			else
				APP_LOG(<< "Export broadcast: left but no right");
#endif
    }
#if LOGGING
		else
			APP_LOG(<< "Export broadcast: no children");
#endif
	
  }
	else
	{
		ExportDirect(m, &msb->lhrq, &msb->lbrq, tag);
		k++;
	}

	if (k)
	{
		msb->n = k;
		mpi_in_flight.push_back(msb);
	}
	else
		delete msb;

	return k;
}

bool 
MessageManager::check_mpi(MessageManager *mm)
{
	Application *app = GetTheApplication();
	
	bool kill_app = false;
	int  read_ready;
	MPI_Status status;

#if LOGGING
	APP_LOG(<< "check_mpi enrty");
#endif

	MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, mm->getP2PComm(), &read_ready, &status);
	if (read_ready)
	{
#ifdef DBG
			APP_LOG(<< "MPI read ready: tag " << status.MPI_TAG);
#endif

		Message *incoming_message = new Message(status);
		if (incoming_message->IsBroadcast())
		{
			
#if LOGGING
			APP_LOG(<< "Received broadcast " << GetTheApplication()->Identify(incoming_message));
#endif

			int nsent = mm->Export(incoming_message);

#if LOGGING
			APP_LOG(<< "passed on broadcast " << GetTheApplication()->Identify(incoming_message));
#endif

			Work *w  = app->Deserialize(incoming_message);
#if LOGGING
			APP_LOG(<< "calling " << GetTheApplication()->Identify(incoming_message) << " CollectiveAction");
#endif
			kill_app = w->CollectiveAction(mm->getCollComm(), GetTheApplication()->GetRank() == incoming_message->header.broadcast_root);
#if LOGGING
			APP_LOG(<< "return from " << GetTheApplication()->Identify(incoming_message) << " CollectiveAction");
#endif
			if (kill_app) killer(); // for debugging
			delete w;

#if LOGGING
			APP_LOG(<< "broadcast handled");
#endif
		}
		else
		{
#if LOGGING
			APP_LOG(<< "Received P2P " << GetTheApplication()->Identify(incoming_message) << " from " << incoming_message->GetSender());
#endif
			
			mm->GetIncomingMessageQueue()->Enqueue(incoming_message);
		}
	}
	else
	{
#if LOGGING
			APP_LOG(<< "nothing from MPI";)
#endif
	}

#if LOGGING
			APP_LOG(<< "check_mpi returns " << kill_app);
#endif
			
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

		int rs;
		MPI_Comm_rank(MPI_COMM_WORLD, &rs);
		mm->SetRank(rs);
		MPI_Comm_size(MPI_COMM_WORLD, &rs);
		mm->SetSize(rs);

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
        o << p;
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

}