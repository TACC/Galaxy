#include <unistd.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <fcntl.h>

using namespace std;

namespace gxy {

class Skt
{
public:

  char *
  Receive()
  {
    int n;

    int rem = sizeof(n);
    unsigned char *p = (unsigned char *)&n;
    while (rem)
    {
      int k = read(cskt, p, rem);
      if (k < 0)
      {
        if (errno != EAGAIN)
          return NULL;
      }
      else
      {
        rem = rem - k;
        p = p + k;
      }
    }

    char *buffer = (char *)malloc(n);

    rem = n;
    p = (unsigned char *)buffer;
    while (rem)
    {
      int k = read(cskt, p, rem);
      if (k < 0)
      {
        if (errno != EAGAIN)
          return NULL;
      }
      else
      {
        rem = rem - k;
        p = p + k;
      }
    }

    return buffer;
  }

  bool
  Send(void *buffer, int n)
  {
    int rem = sizeof(n);
    unsigned char *p = (unsigned char *)&n;
    while (rem)
    {
      int k = write(cskt, p, rem);
      if (k < 0)
      {
        if (errno != EAGAIN)
          return false;
      }
      else
      {
        rem = rem - k;
        p = p + k;
      }
    }

    rem = n;
    p = (unsigned char *)buffer;
    while (rem)
    {
      int k = write(cskt, p, rem);
      if (k < 0)
      {
        if (errno != EAGAIN)
          return false;
      }
      else
      {
        rem = rem - k;
        p = p + k;
      }
    }

    return true;
  }

protected:
  int cskt;
};

class ClientSkt : public Skt
{
public:
  ClientSkt(std::string _host, int _port) 
  {
    host = _host;
    port = _port;
    cskt = -1;
  }

  ~ClientSkt()
  {
    if (IsConnected())
      Disconnect();
  }

  bool IsConnected() { return cskt > 0; }

  bool
  Connect()
  {
    cskt = ::socket(AF_INET, SOCK_STREAM, 0);
    if (cskt < 0)
    {
     perror("ERROR opening socket");
     exit(1);
    }

    struct hostent *server = gethostbyname(host.c_str());
    if (server == NULL)
    {
     fprintf(stderr,"ERROR, no such host\n");
     exit(0);
    }

    struct sockaddr_in serv_addr;
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(port);

    if (::connect(cskt, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
    {
      cskt = -1;
      std::cerr << "error connecting to host " << host << " :: " << port << "\n";
      return false;
    }

    return true;
  }

  void
  Disconnect()
  {
    if (cskt > 0)
    {
      close(cskt);
      cskt = -1;
    }
  }

private:
  std::string host;
  int port;
};


class ServerSkt : public Skt
{
public:
  typedef bool (*ServerSktHandler)(ServerSkt*, void *, char *);

  ServerSkt(int port, ServerSktHandler h, void* p = NULL)
  {
    handler = h;
    ptr = p;

    mskt = ::socket(AF_INET, SOCK_STREAM, 0);
    cskt = -1;

    int flags = fcntl(mskt, F_GETFL, 0);
    flags = (flags | O_NONBLOCK);
    fcntl(mskt, F_SETFL, flags);

    struct sockaddr_in serv_addr;
    bzero((char *) &serv_addr, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port);

    if (::bind(mskt, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
    {
      perror("ERROR on binding");
      exit(1);
    }

    ::listen(mskt,30);
  }

  ~ServerSkt()
  {
    if (! done)
      stop();
  }

  void
  start(bool p = false)
  {
    paused = p;
    pthread_mutex_lock(&mutex);

    done = false;
    pthread_create(&tid, NULL, skt_thread, this);
    thread_running = true;

    pthread_mutex_unlock(&mutex);
  }

  void
  pause()
  {
    paused = true;
  }

  void
  resume()
  {
    pthread_mutex_lock(&mutex);
    paused = false;
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);
  }

  void 
  set_done()
  {
    done = true;
  }

  void 
  stop()
  {
    set_done();
    pthread_join(tid, NULL);
  }

  bool is_running() { return thread_running; }

  static void *
  skt_thread(void *d)
  {
    ServerSkt *me = (ServerSkt *)d;

    me->lock();

    while (! me->done)
    {
      while (me->paused)
        me->wait();

      struct sockaddr_in cli_addr;
      socklen_t cli_len = sizeof(cli_addr);

      me->cskt = accept(me->mskt, (struct sockaddr *)&cli_addr, &cli_len);
      if (me->cskt < 0)
      {
        if (errno != EAGAIN)
        {
          perror("error");
          break;
        }

        struct timespec rem, req = {0, 10000};
        nanosleep(&req, &rem);
      }
      else
      {
        char *buf = me->Receive();

        int status = 1;
        me->Send(&status, sizeof(status));
        
        me->done = (*me->handler)(me, me->ptr, buf);

        close(me->cskt);
        me->cskt = -1;
      }

      me->unlock();
    }

    // std::cerr << "skt thread calling final handler\n";
    (*me->handler)(me, me->ptr, NULL);
    // std::cerr << "skt thread exit\n";

    pthread_exit(NULL);

    return NULL;
  }

private:

  void lock()   { pthread_mutex_lock(&mutex); }
  void unlock() { pthread_mutex_unlock(&mutex); }
  void wait()   { pthread_cond_wait(&cond, &mutex); }
  void signal() { pthread_cond_signal(&cond); }

  pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
  pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
  bool paused;

  pthread_t tid;
  bool thread_running = false;
  ServerSktHandler handler;
  void *ptr;
  int mskt;
  bool done;
};

}
