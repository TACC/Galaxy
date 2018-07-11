#pragma once 

#include <iostream>
#include <deque>
#include <pthread.h>
#include "Message.h"

namespace gxy
{
class MessageQ {
public:
  MessageQ(const char *n) : name(n)
  {
    pthread_mutex_init(&lock, NULL);
    pthread_cond_init(&signal, NULL);
    running = true;
  }

  ~MessageQ()
  {
    running = false;
    pthread_cond_broadcast(&signal);
  }

  void Kill();

  void Enqueue(Message *w);
  Message *Dequeue();
  int IsReady();

	int size() { return workq.size(); }

	void printContents();

private:
  const char *name;

  pthread_mutex_t lock;
  pthread_cond_t signal;
  bool running;

  std::deque<Message *> workq;
};

MessageQ *GetIncomingMessageQueue();
MessageQ *GetOutgoingMessageQueue();
}
