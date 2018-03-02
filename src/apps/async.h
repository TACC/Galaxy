#define START					347543829
#define MOUSEDOWN  		384991286
#define MOUSEMOTION 	847387232
#define QUIT	 				298374284
#define DEBUG	 				843994344
#define STATS	 				735462177
#define SYNCHRONIZE		732016373
#define RENDER_ONE	  873221232
#define RESET_CAMERA	777629294

enum cam_mode
{
  OBJECT_CENTER, EYE_CENTER
};

struct mouse_down
{
	int op;
  int k, s;
	float x, y;
};

struct mouse_motion
{
	int op;
	float x, y;
};

struct start
{
	int op;
	cam_mode mode;
	int width, height;
	char name[1];
};
