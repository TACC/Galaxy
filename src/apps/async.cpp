#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <glut.h>
#include <pthread.h>
#include "ImageWriter.h"

using namespace std;

#include "quat.h"

#include "Application.h"
#include "Renderer.h"
#include "AsyncRendering.h"

#include <ospray/ospray.h>

#define WIDTH  500
#define HEIGHT 500

int width = WIDTH, height = HEIGHT;
float X0, Y0;
float X1, Y1;

volatile int frame = -1;

int  *pargc;
char **pargv;

string statefile("");

ImageWriter image_writer("async");

AsyncRenderingP theRendering = NULL;
RenderingSetP 	theRenderingSet = NULL;
CameraP 				theCamera = NULL;
VisualizationP 	theVisualization = NULL;
DatasetsP 			theDatasets = NULL;

float *pixels = NULL;
int   *frameids = NULL;
bool  render_one = false;

enum _state
{
	RUNNING,
	QUITTING,
	DONE
} state;

pthread_mutex_t state_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  state_wait = PTHREAD_COND_INITIALIZER;

_state
get_state()
{
  pthread_mutex_lock(&state_lock);
  _state s = state;
	pthread_mutex_unlock(&state_lock);
	return s;
}

void
set_state(_state s)
{
  pthread_mutex_lock(&state_lock);
  state  = s;
	pthread_cond_signal(&state_wait);
	pthread_mutex_unlock(&state_lock);
}

void
wait_state(_state s)
{
	pthread_mutex_lock(&state_lock);
	while (s != state)
		pthread_cond_wait(&state_wait, &state_lock);
	pthread_mutex_unlock(&state_lock);
}

class Debug
{
public:
  Debug(const char *executable, bool attach)
  {
    cerr << "Debug ctor\n";
    bool dbg = true;
    std::stringstream cmd;
    pid_t pid = getpid();

    if (attach)
      std::cerr << "Attach to PID " << pid << "\n";
    else
    {
      cmd << "~/dbg_script " << executable << " " << pid << " &";
      system(cmd.str().c_str());
    }

    while (dbg)
      sleep(1);

    std::cerr << "running\n";
  }
};

void
draw(void)
{
	if (pixels)
	{
		glDrawPixels(width, height, GL_RGBA, GL_FLOAT, pixels);
		glutSwapBuffers();
	}
}

void
animate(void)
{
  glutPostRedisplay();
}

static int inum = 0;
	
void
keyboard(unsigned char ch, int x, int y)
{
  switch (ch) {
		case 0x52:
			render_one = true;
			break;

		case 0x53:
		case 0x73:
				image_writer.Write(width, height, pixels);
				break;

    case 0x1B: 
			set_state(QUITTING);
			wait_state(DONE);
      GetTheApplication()->QuitApplication();
      GetTheApplication()->Wait();
      exit(0);
      break;

		case 0x40:
			glutPostRedisplay();
			break;
  }
}

#define QUIT    1

void
menu(int item)
{
  switch (item) {
		case 0x52:
			render_one = true;
			break;

		case 0x53:
		case 0x73:
				image_writer.Write(width, height, pixels);
				break;

    case 0x1B: 
			set_state(QUITTING);
			wait_state(DONE);

      GetTheApplication()->QuitApplication();
      GetTheApplication()->Wait();

      exit(0);
      break;
  }
}

void
mousefunc(int k, int s, int x, int y)
{
  if (s == GLUT_DOWN)
  {
    X0 = -1.0 + 2.0*(float(x)/WIDTH);
    Y0 = -1.0 + 2.0*(float(y)/HEIGHT);
  }
}
  
void
motionfunc(int x, int y)
{
	X1 = -1.0 + 2.0*(float(x)/WIDTH);
	Y1 = -1.0 + 2.0*(float(y)/HEIGHT);
}

void
glut_loop()
{
  glutInit(pargc, pargv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
  glutInitWindowSize(width, height);
  glutCreateWindow("render");
  glutIdleFunc(draw);
  glutDisplayFunc(draw);
  glutMotionFunc(motionfunc);
  glutMouseFunc(mousefunc);
  glutKeyboardFunc(keyboard);
  glutCreateMenu(menu);
  glutAddMenuEntry("Quit", QUIT);
  glutAttachMenu(GLUT_RIGHT_BUTTON);
  glClearDepth(1.0);
  glClearColor(0.0, 0.0, 0.0, 0.0);
  glMatrixMode(GL_PROJECTION);
  glOrtho(-1, 1, -1, 1, -1, 1);
  glMatrixMode(GL_MODELVIEW);
  glRasterPos2i(-1, -1);
  frame = 1;
  glutMainLoop();
}

void *
render_thread(void *d)
{
  RendererP theRenderer = Renderer::NewP();
  Document *doc = GetTheApplication()->OpenInputState(statefile);
  theRenderer->LoadStateFromDocument(*doc);

  vector<CameraP> theCameras = Camera::LoadCamerasFromJSON(*doc);
  theCamera = theCameras[0];

  vec3f scaled_viewdirection, center, viewpoint, viewdirection, viewup;
  theCamera->get_viewpoint(viewpoint);
  theCamera->get_viewdirection(viewdirection);
  theCamera->get_viewup(viewup);
  add(viewpoint, viewdirection, center);
 
  float aov, viewdistance = len(viewdirection);
  theCamera->get_angle_of_view(aov);

  normalize(viewdirection);
  normalize(viewup);

  scaled_viewdirection = viewdirection;
  scale(viewdistance, scaled_viewdirection);

  vec3f viewright;
  cross(viewdirection, viewup, viewright);
  cross(viewright, viewdirection, viewup);

  vec3f y(0.0, 1.0, 0.0);
  float ay = acos(dot(y, viewup));

  vec4f current_rotation;
  axis_to_quat(viewdirection, ay, current_rotation);

  theDatasets = Datasets::NewP();
  theDatasets->LoadFromJSON(*doc);
  theDatasets->Commit();

  vector<VisualizationP> theVisualizations = Visualization::LoadVisualizationsFromJSON(*doc);
  theVisualization = theVisualizations[0];
	theVisualization->Commit(theDatasets);

	theRendering = AsyncRendering::NewP();
	theRendering->SetBuffers(pixels, frameids);
	theRendering->SetTheOwner(0);
	theRendering->SetTheSize(width, height);
	theRendering->SetTheDatasets(theDatasets);
	theRendering->SetTheVisualization(theVisualization);
	theRendering->SetTheCamera(theCamera);
	theRendering->Commit();

	theRenderingSet = RenderingSet::NewP();
	theRenderingSet->AddRendering(theRendering);
	theRenderingSet->Commit();

	bool first = true;

	while (get_state() == RUNNING)
	{
		if (render_one || first || (X0 != X1) || (Y0 != Y1))
		{
			first = false;
			if ((X0 != X1) || (Y0 != Y1))
			{
				vec4f this_rotation;
				trackball(this_rotation, X0, Y0, X1, Y1);
			
				vec4f next_rotation;
				add_quats(this_rotation, current_rotation, next_rotation);
				current_rotation = next_rotation;

				vec3f y(0.0, 1.0, 0.0);
				vec3f z(0.0, 0.0, 1.0);

				rotate_vector_by_quat(y, current_rotation, viewup);
				rotate_vector_by_quat(z, current_rotation, viewdirection);

				scaled_viewdirection = viewdirection;
				scale(viewdistance, scaled_viewdirection);
				sub(center, scaled_viewdirection, viewpoint);
			}

#if 0
#define PVEC(n, v) \
			std::cerr << n << ": " << v.x << " " << v.y << " " << v.z << "\n";

			PVEC("SVD", scaled_viewdirection)
			PVEC("VP", viewpoint)
			PVEC("UP", viewup)
			std::cerr << "AOV: " << aov << "\n";
#endif

      theCamera->set_viewdirection(scaled_viewdirection);
      theCamera->set_viewpoint(viewpoint);
      theCamera->set_viewup(viewup);
      theCamera->set_angle_of_view(aov);
      theCamera->Commit();
#if 0

      theRendering->SetTheCamera(theCamera);
      theRendering->Commit();

      theRenderingSet->Commit();
#endif

			std::cerr << "+";

			if (render_one)
			{
				GetTheApplication()->SyncApplication();
				render_one = false;
			}

			theRenderer->Render(theRenderingSet);

			std::cerr << "0";
			
			X0 = X1; Y0 = Y1;
		}
	}

	set_state(DONE);
}

void
syntax(char *a)
{
  cerr << "syntax: " << a << " [options] statefile\n";
  cerr << "options:\n";
  cerr << "  -D         run debugger\n";
  cerr << "  -A         wait for attachment\n";
  cerr << "  -s w h     image size (512 x 512)\n";
  exit(1);
}

int
main(int argc, char *argv[])
{
  bool dbg = false, atch = false;

  pargc = &argc;
  pargv = argv;

  ospInit(pargc, (const char **)argv);

  Application theApplication(&argc, &argv);
  theApplication.Start();

	AsyncRendering::RegisterClass();

  for (int i = 1; i < argc; i++)
  {
    if (!strcmp(argv[i], "-A")) dbg = true, atch = true;
    else if (!strcmp(argv[i], "-D")) dbg = true, atch = false;
    else if (!strcmp(argv[i], "-s"))
    {
      width  = atoi(argv[++i]);
      height = atoi(argv[++i]);
    }
    else if (statefile == "")
      statefile = argv[i];
    else
      syntax(argv[0]);
  }

  if (statefile == "")
    syntax(argv[0]);

  Debug *d = dbg ? new Debug(argv[0], atch) : NULL;

  Renderer::Initialize();
  GetTheApplication()->Run();

  int mpiRank = GetTheApplication()->GetRank();
  int mpiSize = GetTheApplication()->GetSize();

  if (mpiRank == 0)
  {
		pixels = new float[width*height*4];
		frameids = new int[width*height];

		for (int i = 0; i < width*height*4; i++) pixels[i] = 0.0;
		memset(frameids, 0, width*height*sizeof(int));

    pthread_t tid;
    pthread_create(&tid, NULL, render_thread, NULL);

    glut_loop();
  }
  else
    GetTheApplication()->Wait();

  return 0;
}
