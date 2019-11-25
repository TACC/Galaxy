#include <iostream>

#include <QInputEvent>

static int xx = 0;

#include "GxyRenderWindow.hpp"

#include <QJsonDocument>
#include <QSurfaceFormat>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QVBoxLayout>

static long
my_time()
{
  timespec s;
  clock_gettime(CLOCK_REALTIME, &s);
  return 1000000000*s.tv_sec + s.tv_nsec;
}

GxyRenderWindow::GxyRenderWindow(std::string rid)
{
  pthread_mutex_init(&lock, NULL);
  renderer_id = rid;

  resize(512, 512);
  setFocusPolicy(Qt::StrongFocus);

  kill_threads = false;
  rcvr_tid = NULL;
  ager_tid = NULL;
}

GxyRenderWindow::~GxyRenderWindow()
{
  if (rcvr_tid)
  {
    kill_threads = true;
    pthread_join(rcvr_tid, NULL);
    rcvr_tid = NULL;
  }

  if (ager_tid)
  {
    pthread_join(ager_tid, NULL);
    ager_tid = NULL;
  }

  if (pixels) free(pixels);
  negative_pixels = NULL;

  if (negative_pixels) free(negative_pixels);
  negative_pixels = NULL;

  if (frameids) free(frameids);
  frameids = NULL;

  if (negative_frameids) free(negative_frameids);
  negative_frameids = NULL;

  if (frame_times) free(frame_times);
  frame_times = NULL;
}

void
GxyRenderWindow::setCamera(Camera&c)
{
  camera = c; 
  trackball.reset();

  current_direction = camera.getDirection();
  current_up = camera.getUp();

  gxy::vec3f point = camera.getPoint();
  current_center = point + current_direction;

  gxy::vec2i size = c.getSize();
  resize(size.x, size.y);

  sendCamera();
}


void
GxyRenderWindow::onLightingChanged(LightingEnvironment& l)
{
  lighting = l;
}

void
GxyRenderWindow::onVisUpdate(std::shared_ptr<GxyVis> v)
{
  if (v)
  {
    Visualization[v->get_origin()] = v;
    for (auto vis : Visualization)
    {
      std::cerr << vis.first << " =======================\n";
      vis.second->print();
    }
  }
}

void
GxyRenderWindow::render()
{
  if (getTheGxyConnectionMgr()->IsConnected())
  {
    QJsonObject renderJson;
    renderJson["cmd"] = "gui::render";
    renderJson["id"] = renderer_id.c_str();

    QJsonDocument doc(renderJson);
    QByteArray bytes = doc.toJson(QJsonDocument::Compact);
    QString qs = QLatin1String(bytes);

    std::string msg = qs.toStdString();
    getTheGxyConnectionMgr()->CSendRecv(msg);
  }
}

void
GxyRenderWindow::onVisRemoved(std::string id)
{
  Visualization.erase(id);
}

std::string
GxyRenderWindow::cameraString()
{
  QJsonObject cameraJson;
  cameraJson["Camera"] = camera.save();
  QJsonDocument doc(cameraJson);
  QByteArray bytes = doc.toJson(QJsonDocument::Compact);
  QString s = QLatin1String(bytes);

  return s.toStdString();
}

std::string
GxyRenderWindow::visualizationString()
{
  QJsonObject visualizationJson;

  QJsonObject v;
  v["Lighting"] = lighting.save();

  QJsonArray operatorsJson;
  for (auto vis : Visualization)
  {
    QJsonObject operatorJson;
    vis.second->save(operatorJson);
    operatorsJson.push_back(operatorJson);
  }
  v["operators"] = operatorsJson;

  visualizationJson["Visualization"] = v;

  QJsonDocument doc(visualizationJson);
  QByteArray bytes = doc.toJson(QJsonDocument::Compact);
  QString s = QLatin1String(bytes);
  return s.toStdString();
}

void
GxyRenderWindow::mousePressEvent(QMouseEvent *event) 
{
  grabMouse();
  
  switch(event->button())
  {
    case Qt::LeftButton:   button = 0; break;
    case Qt::MiddleButton: button = 1; break;
    case Qt::RightButton:  button = 2; break;
    default: button = 3;
  }

  x0 = x1 = -1.0 + 2.0*(float(event->x())/width);
  y0 = y1 = -1.0 + 2.0*(float(event->y())/height);
}

void
GxyRenderWindow::mouseMoveEvent(QMouseEvent *event) 
{
  x1 = -1.0 + 2.0*(float(event->x())/width);
  y1 = -1.0 + 2.0*(float(event->y())/height);
  
  if (button == 0)
  { 
    trackball.spin(x0, y0, x1, y1);
    
    gxy::vec3f direction = trackball.rotate_vector(current_direction);
    gxy::vec3f up = trackball.rotate_vector(current_up);
    gxy::vec3f point = current_center - direction;
    
    camera.setPoint(point);
    camera.setDirection(direction);
    camera.setUp(up);
    sendCamera();
    render();
    // sleep(2);
  }
  else if (button == 1)
  { 
    //frame_direction = current_direction;
    //frame_up = current_up;
    //gxy::vec3f step = current_direction;
    //gxy::scale((y1 - y0), step);
    //frame_center = current_center = current_center + step;
  }
}

void
GxyRenderWindow::mouseReleaseEvent(QMouseEvent *event) 
{
  button = -1;
  releaseMouse();
}

void
GxyRenderWindow::keyPressEvent(QKeyEvent *event) 
{
  std::string key = event->text().toStdString();
  if (key == "r")
    render();
  else if (key == "R")
  {
    std::stringstream ss;
    ss << "renderMany 10";
    std::string msg = ss.str();
    getTheGxyConnectionMgr()->CSendRecv(msg);
  }
  else if (key == "r")
  {
    std::string msg("renderMany 10");
    getTheGxyConnectionMgr()->CSendRecv(msg);
    std::cerr << "got reply: " << msg << "\n";
  }
  else if (key == "C")
  {
    for (int i = 0; i < 10; i++)
    {
      sendCamera();
      std::string msg("render");
      getTheGxyConnectionMgr()->CSendRecv(msg);
      std::cerr << "got reply: " << msg << "\n";
    }
  }
  else
  {
    Q_EMIT characterStrike(key.c_str()[0]);
  }
}
    
void
GxyRenderWindow::sendCamera()
{
  if (getTheGxyConnectionMgr()->IsConnected())
  {
    setSize(camera);

    QJsonObject cameraJson;
    cameraJson["Camera"] = camera.save();

    cameraJson["cmd"] = "gui::camera";
    cameraJson["id"] = renderer_id.c_str();

    QJsonDocument doc(cameraJson);
    QByteArray bytes = doc.toJson(QJsonDocument::Compact);
    QString qs = QLatin1String(bytes);

    std::string msg = qs.toStdString();
    getTheGxyConnectionMgr()->CSendRecv(msg);
  }
}

void 
GxyRenderWindow::manageThreads(bool state)
{
  if (state)
  {
    if (rcvr_tid != NULL)
      std::cerr << "threads are running at time of connection\n";
    else
    {
      kill_threads = false;
      pthread_create(&rcvr_tid, NULL, pixel_receiver_thread, (void *)this);
      pthread_create(&ager_tid, NULL, pixel_ager_thread, (void *)this);
    }
  }
  else
  {
    if (rcvr_tid == NULL)
      std::cerr << "threads are NOT running at time of disconnection\n";
    else
    {
      kill_threads = true;
      pthread_join(rcvr_tid, NULL);
      pthread_join(ager_tid, NULL);
      rcvr_tid = ager_tid = NULL;
    }
  }
}

void
GxyRenderWindow::initializeGL()
{
  initializeOpenGLFunctions();

  glClearDepth(1.0);
  glClearColor(0.0, 0.0, 0.0, 0.0);
  glMatrixMode(GL_PROJECTION);
  glOrtho(-1, 1, -1, 1, -1, 1);
  glMatrixMode(GL_MODELVIEW);
  glRasterPos2i(-1, -1);
}

void
GxyRenderWindow::paintGL()
{
  float dpr = devicePixelRatio();
  glPixelZoom(dpr, dpr);
  glDrawPixels(width, height, GL_RGBA, GL_FLOAT, pixels);
}

void
GxyRenderWindow::resizeGL(int w, int h)
{
  if (width != w || height != h)
  {
    width = w;
    height = h;

    camera.setSize(width, height);

    resize(w, h);

    if (pixels) free(pixels);
    if (negative_pixels) free(negative_pixels);
    if (frameids) free(frameids);
    if (negative_frameids) free(negative_frameids);
    if (frame_times) free(frame_times);

    pixels             = (float *)malloc(width*height*4*sizeof(float));
    negative_pixels    = (float *)malloc(width*height*4*sizeof(float));
    frameids           = (int *)malloc(width*height*sizeof(int));
    negative_frameids  = (int *)malloc(width*height*sizeof(int));
    frame_times        = (long *)malloc(width*height*sizeof(long));

    memset(frameids, 0, width*height*sizeof(int));
    memset(negative_frameids, 0, width*height*sizeof(int));

    long now = my_time();
    for (int i = 0; i < width*height; i++)
      frame_times[i] = now;

    for (int i = 0; i < 4*width*height; i++)
    {
      pixels[i] = (i & 0x3) == 0x3 ? 1.0 : 0.0;
      negative_pixels[i] = (i & 0x3) == 0x3 ? 1.0 : 0.0;
    }

    sendCamera();
    render();
  }
}

void
GxyRenderWindow::addPixels(gxy::Pixel *p, int n, int frame)
{
  pthread_mutex_lock(&lock);

  long now = my_time();

  if (frame >= current_frame)
  {
    //  If frame is strictly greater than current_frame then
    //  updating it will kick the ager to begin aging
    //  any pixels from prior frames.   We want to start the
    //  aging process from the arrival of the first contribution
    //  to the new frame rather than its updated time.
		
    if (frame > current_frame)
    {
      current_frame = frame;

      for (int offset = 0; offset < width*height; offset++)
        frame_times[offset] = now;
    }

    // Only bump current frame IF this is a positive pixel

    for (int i = 0; i < n; i++, p++)
    {
      if (p->x >= 0 && p->x < width && p->y > 0 && p->y < height)
      {
        size_t offset = p->y*width + p->x;
        float *pix = pixels + (offset<<2);
        float *npix = negative_pixels + (offset<<2);

        // If its a sample from the current frame, add it in.
        // 
        // If its a sample from a LATER frame then two possibilities:
        // its a negative sample, in which case we DON'T want to update
        // the visible pixel, so we stash it. If its a POSITIVE sample
        // we stuff the visible pixel with the sample and any stashed
        // negative samples.

        if (frameids[offset] == frame)
        {
          *pix++ += p->r;
          *pix++ += p->g;
          *pix++ += p->b;
        }
        else
        {
          if (p->r < 0.0 || p->g < 0.0 || p->b < 0.0)
          {
            // If its a NEGATIVE sample and...

            if (negative_frameids[offset] == frame)
            {
              // from the current frame, we add it to the stash
              *npix++ += p->r;
              *npix++ += p->g;
              *npix++ += p->b;
            }
            else if (negative_frameids[offset] < frame)
            {
              // otherwise its from a LATER frame, so we init the stash so if
              // we receive any POSITIVE samples we can add it in.
              negative_frameids[offset] = frame;
              *npix++ = p->r;
              *npix++ = p->g;
              *npix++ = p->b;
            }
          }
          else
          {
            // its a POSITIVE sample from a NEW frame, so we stuff the visible
            // pixel with the new sample and any negative samples that arrived 
            // first

            frameids[offset] = frame;
            if (current_frame == negative_frameids[offset])
            {
              *pix++ = (*npix + p->r);
              *npix = 0.0;
              npix++;
              *pix++ = (*npix + p->g);
              *npix = 0.0;
              npix++;
              *pix++ = (*npix + p->b);
              *npix = 0.0;
              npix++;
            }
            else
            {
              *pix++ = p->r;
              *pix++ = p->g;
              *pix++ = p->b;
            }
          }
        }
      }
    }
  }

  pthread_mutex_unlock(&lock);
}

void *
GxyRenderWindow::pixel_receiver_thread(void *d)
{
  GxyRenderWindow *wndw = (GxyRenderWindow *)d;
  GxyConnectionMgr *connection = getTheGxyConnectionMgr();

  while (! wndw->kill_threads)
  {
    if (connection->DWait(0.1))
    {
      char *buf; int n;
      connection->DRecv(buf, n);

      char *ptr = buf;
      int knt = *(int *)ptr;
      ptr += sizeof(int);
      int frame = *(int *)ptr;
      ptr += sizeof(int);
      int sndr = *(int *)ptr;
      ptr += sizeof(int);
      gxy::Pixel *p = (gxy::Pixel *)ptr;

      wndw->addPixels(p, knt, frame);

      free(buf);
    }
  }

  std::cerr << "pixel_receiver_thread stopping\n";

  pthread_exit(NULL);
}

void *
GxyRenderWindow::pixel_ager_thread(void *d)
{
  GxyRenderWindow *wndw = (GxyRenderWindow *)d;
  
  while (! wndw->kill_threads)
  {
    struct timespec rm, tm = {0, 100000000};
    nanosleep(&tm, &rm);
    wndw->FrameBufferAger();
  }

  pthread_exit(NULL);
}

void
GxyRenderWindow::FrameBufferAger()
{
  if (pixels && frameids && frame_times)
  {
    pthread_mutex_lock(&lock);

    long now = my_time();
    int n = (height/2)*width + (width/2);

    for (int offset = 0; offset < width*height; offset++)
    {
      int fid = frameids[offset];
      if (fid > 0 && fid < current_frame)
      {
        long tm = frame_times[offset];
        float sec = (now - tm) / 1000000000.0;

        float *pix = pixels + (offset*4);
        if (sec > max_age)
        {
          if (sec > (max_age + fadeout))
          {
            frame_times[offset] = now;
            frameids[offset] = current_frame;
            *pix++ = 0.0, *pix++ = 0.0, *pix++ = 0.0, *pix++ = 1.0;
          }
          else
            *pix++ *= 0.9, *pix++ *= 0.9, *pix++ *= 0.9, *pix++ = 1.0;
        }
      }
    }

    pthread_mutex_unlock(&lock);
  }
}

