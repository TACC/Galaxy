#include <iostream>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
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

#define BUFSZ 10240

class Skt
{
public:

  char *
  Receive()
  {
    char *buf = ReceiveRaw();

    if (buf)
    {
      int status = 1;
      SendRaw(&status, sizeof(status));
    }

    return buf;
  }

  char *
  ReceiveRaw()
  {
    int n = 9999999;

    int rem = sizeof(n);
    unsigned char *p = (unsigned char *)&n;
    while (rem)
    {
      int k = read(cskt, p, rem);
      if ((k < 0 && errno != EAGAIN) || k == 0)
        return NULL;
      else if (k > 0)
      {
        rem = rem - k;
        p = p + k;
      }
    }

    // std::cerr << "reading " << n << " bytes on skt " << cskt << "\n";
    char *buffer = (char *)malloc(n);

    rem = n;
    p = (unsigned char *)buffer;
    while (rem)
    {
      int l = (rem > 1024) ? 1024 : rem;
      int k = read(cskt, p, l);
      if ((k < 0 && errno != EAGAIN) || k == 0)
        return NULL;
      else if (k > 0)
      {
        rem = rem - k;
        p = p + k;
      }
    }

    return buffer;
  }

  bool 
  Send(char *buffer)
  {
    return Send((void *)buffer, strlen(buffer)+1);
  }

  bool
  Send(std::string buffer)
  {
    return Send((void *)buffer.c_str(), buffer.size()+1);
  }

  bool
  Send(void *buffer, int n)
  {
    int r = 0;

    if (SendRaw(buffer, n))
    {
      char *rply = ReceiveRaw();
      if (rply)
      {
        r = *(int *)rply;
        free(rply);
      }
    }

    return r == 1;
  }

  bool 
  SendRaw(char *buffer)
  {
    return SendRaw((void *)buffer, strlen(buffer)+1);
  }

  bool
  SendRaw(std::string buffer)
  {
    return SendRaw((void *)buffer.c_str(), buffer.size()+1);
  }

  bool
  SendRaw(void *buffer, int n)
  {
    // std::cerr << "sending " << n << " bytes on skt " << cskt << "\n";

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
      int l = (rem > 1024) ? 1024 : rem;
      int k = write(cskt, p, l);
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
  ClientSkt()
  {
    host = "localhost";
    port = 1900;
    cskt = -1;
  }

  ClientSkt(std::string _host, int _port) 
  {
    host = _host;
    port = _port;
    cskt = -1;
  }

  ~ClientSkt()
  {
    // std::cerr << "Client socket dtor... " << cskt << "\n";
    if (IsConnected())
      Disconnect();
  }

  bool IsConnected() { return cskt > 0; }

  bool
  Connect(std::string h, int p)
  {
    host = h;
    port = p;
    return Connect();
  }

  bool
  Connect()
  {
    cskt = ::socket(AF_INET, SOCK_STREAM, 0);
    if (cskt < 0)
    {
     perror("ERROR opening socket");
     return false;
    }

    struct hostent *server = gethostbyname(host.c_str());
    if (server == NULL)
    {
     fprintf(stderr,"ERROR, no such host\n");
     return false;
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

    // std::cerr << "port " << port << " connected to " << cskt << "\n";
    return true;
  }

  void
  Disconnect()
  {
    if (cskt > 0)
    {
      // std::cerr << "Client socket Disconnect... " << cskt << "\n";
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

    // std::cerr << "Master socket " << mskt << "\n";

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
      std::stringstream ss;
      ss << "Error on binding... port " << port;
      perror(ss.str().c_str());
      exit(1);
    }

    ::listen(mskt,30);

    // std::cerr << "port " << port << " listening on socket " << mskt << "\n";
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
    lock();

    done = false;
    pthread_create(&tid, NULL, skt_thread, this);
    thread_running = true;

    unlock();
  }

  void
  pause()
  {
    lock();
    paused = true;
    signal();
    unlock();
  }

  void
  resume()
  {
    lock();
    paused = false;
    signal();
    unlock();
  }

  void 
  set_done()
  {
    lock();
    done = true;
    signal();
    unlock();
  }

  void 
  stop()
  {
    lock();
    done = true;
    signal();
    unlock();

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
      while (me->paused && !me->done)
        me->wait();

      if (me->done)
        break;

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

        struct timespec rem, req = {0, 10000000};
        nanosleep(&req, &rem);
      }
      else
      {
        // std::cerr << "Server " << me->mskt << " accepts on socket " << me->cskt << "\n";
        char *buf;

        while((buf = me->Receive()) != NULL && !me->paused)
          me->paused = (*me->handler)(me, me->ptr, buf) || me->done;

        // std::cerr << "Server closes socket " << me->cskt << "\n";
        close(me->cskt);
        me->cskt = -1;
      }

      me->unlock();
    }

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
