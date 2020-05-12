#include "GxyRenderWindow.hpp"
#include "GxyRenderWindowMgr.hpp"
#include "Pixel.h"

static GxyRenderWindowMgr *_theGxyRenderWindowMgr = NULL;

GxyRenderWindowMgr::GxyRenderWindowMgr()
{
  _theGxyRenderWindowMgr = this;
}

GxyRenderWindowMgr::~GxyRenderWindowMgr()
{
  if (rcvr_tid)
  {
    kill_threads = true;
    pthread_join(rcvr_tid, NULL);
  }

  _theGxyRenderWindowMgr = NULL;;
}

void
GxyRenderWindowMgr::StartReceiving()
{
  kill_threads = false;
  pthread_create(&rcvr_tid, NULL, pixel_receiver_thread, (void *)this);
}

void
GxyRenderWindowMgr::StopReceiving()
{
  kill_threads = true;
  pthread_join(rcvr_tid, NULL);
}

void
GxyRenderWindowMgr::RegisterRenderWindow(std::string id, GxyRenderWindow* w)
{
  window_map[id] = w;
}

GxyRenderWindow *
GxyRenderWindowMgr::GetRenderWindow(std::string id)
{
  std::map<std::string, GxyRenderWindow *>::iterator it = window_map.find(id);
  if (it == window_map.end()) return NULL;
  else return it->second;
}

void
GxyRenderWindowMgr::Remove(std::string id)
{
  window_map.erase(id);
}

void *
GxyRenderWindowMgr::pixel_receiver_thread(void *d)
{
  GxyRenderWindowMgr *mgr = (GxyRenderWindowMgr *)d;
  GxyConnectionMgr *connection = getTheGxyConnectionMgr();

  while (! mgr->kill_threads)
  {
    if (connection->DWait(0.1))
    {
      char *buf; int n;
      connection->DRecv(buf, n);

      char *ptr = buf;

      std::string id = ptr;
      ptr += (id.size() + 1);

      GxyRenderWindow *wndw = mgr->GetRenderWindow(id);

      if (wndw)
      {
        int knt = *(int *)ptr;
        ptr += sizeof(int);
        int frame = *(int *)ptr;
        ptr += sizeof(int);
        int sndr = *(int *)ptr;
        ptr += sizeof(int);
        gxy::Pixel *p = (gxy::Pixel *)ptr;

        wndw->addPixels(p, knt, frame);
      }
      free(buf);
    }
  }

  std::cerr << "pixel_receiver_thread stopping\n";
  pthread_exit(NULL);
}

GxyRenderWindowMgr *getTheGxyRenderWindowMgr() { return  _theGxyRenderWindowMgr; }
