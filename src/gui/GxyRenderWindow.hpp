#pragma once

#include <QOpenGLWidget>
#include <QMainWindow>
#include <QOpenGLFunctions>
#include <QGroupBox>
#include <QGridLayout>

#include "GxyConnectionMgr.hpp"
#include "Camera.hpp"
#include "Lights.hpp"
#include "GxyVis.hpp"

#include "Pixel.h"
#include "trackball.hpp"

class GxyRenderWindow;

class GxyOpenGLWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
  Q_OBJECT

public:
  GxyOpenGLWidget(QWidget *parent);
  ~GxyOpenGLWidget();

  void manageThreads(bool);
  void addPixels(gxy::Pixel *p, int n, int frame);
  void Update() { update(); }
  void setSize(int w, int h) { width = w; height = h; } 

protected:
  void resizeGL(int w, int h) override;
  void paintGL() override;
  void initializeGL() override;

  void resizeEvent(QResizeEvent *e) override
  {
    std::cerr << "resize!\n";
    QOpenGLWidget::resizeEvent(e);
  }

private:
  int width, height;

  static void *pixel_receiver_thread(void *d);
  static void *pixel_ager_thread(void *d);
  void FrameBufferAger();

  bool kill_threads;
  pthread_mutex_t lock;
  pthread_t rcvr_tid = NULL;
  pthread_t ager_tid = NULL;

  int current_frame = -1;
  float max_age = 3.0;
  float fadeout = 1.0;

  float* pixels = NULL;
  float* negative_pixels = NULL;
  int* frameids = NULL;
  int* negative_frameids = NULL;
  long* frame_times = NULL;
};

class GxyRenderWindow : public QMainWindow
{
  Q_OBJECT

public:
  GxyRenderWindow(std::string rid);
  ~GxyRenderWindow();

  void render();
  void Update() { if (oglWidget) oglWidget->Update(); }

  void manageThreads(bool b) { oglWidget->manageThreads(b); }
  void setCamera(Camera&);

  void setSize(int w, int h)
  {
    if (w != width || h != height)
    {
      width = w;
      height = h;

      oglWidget->setSize(w, h);

      if (getTheGxyConnectionMgr()->IsConnected())
      {
        std::stringstream ss;
        ss << "window " << width << " " << height;
        std::string s = ss.str();
        getTheGxyConnectionMgr()->CSendRecv(s);
      }
    }
  }

  void setSize(Camera& c)
  {
    gxy::vec2i s = c.getSize();
    setSize(s.x, s.y);
  }

protected:

  void sendCamera();

  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void keyPressEvent(QKeyEvent *event) override;

  std::string cameraString();
  std::string visualizationString();

signals:
  void characterStrike(char c);

public Q_SLOTS:
  void onLightingChanged(LightingEnvironment&);
  void onVisUpdate(std::shared_ptr<GxyVis>);
  void onVisRemoved(std::string);

private:
  float x0, x1, y0, y1;

  gxy::vec3f current_direction;
  gxy::vec3f current_center;
  gxy::vec3f current_up;

  int width, height;

  Camera camera;
  LightingEnvironment lighting;
  GxyOpenGLWidget *oglWidget;
  std::map<std::string, std::shared_ptr<GxyVis>> Visualization;
  int button = -1;
  std::string renderer_id;

  gxy::Trackball trackball;
};
