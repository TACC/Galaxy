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

class GxyRenderWindow;

class GxyOpenGLWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
  Q_OBJECT

public:
  GxyOpenGLWidget(GxyRenderWindow *mw);
  ~GxyOpenGLWidget();

  void render();
  void addPixels(gxy::Pixel *p, int n, int frame);
  void Update() { update(); }

protected:
  void resizeGL(int w, int h) override;
  void paintGL() override;
  void initializeGL() override;

private:
  static void *pixel_receiver_thread(void *d);
  static void *pixel_ager_thread(void *d);
  void FrameBufferAger();

  GxyRenderWindow *m_top;

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

  int width = -1, height = -1;
};

class GxyRenderWindow : public QMainWindow
{
  Q_OBJECT

public:
  GxyRenderWindow();

  void render() { if (oglWidget) oglWidget->render(); }
  void Update() { if (oglWidget) oglWidget->Update(); }

protected:

  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void keyPressEvent(QKeyEvent *event) override;

  std::string cameraString();
  std::string visualizationString();

public Q_SLOTS:
  void onCameraChanged(Camera&);
  void onLightingChanged(LightingEnvironment&);
  void onVisUpdate(std::shared_ptr<GxyVis>);
  void onVisRemoved(std::string);

private:
  float x0, x1, y0, y1;
  Camera camera;
  LightingEnvironment lighting;
  GxyOpenGLWidget *oglWidget;
  std::map<std::string, std::shared_ptr<GxyVis>> Visualization;
  int button = -1;
};
