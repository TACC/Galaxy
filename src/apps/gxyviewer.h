// ========================================================================== //
// Copyright (c) 2014-2020 The University of Texas at Austin.                 //
// All rights reserved.                                                       //
//                                                                            //
// Licensed under the Apache License, Version 2.0 (the "License");            //
// you may not use this file except in compliance with the License.           //
// A copy of the License is included with this software in the file LICENSE.  //
// If your copy does not contain the License, you may obtain a copy of the    //
// License at:                                                                //
//                                                                            //
//     https://www.apache.org/licenses/LICENSE-2.0                            //
//                                                                            //
// Unless required by applicable law or agreed to in writing, software        //
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT  //
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.           //
// See the License for the specific language governing permissions and        //
// limitations under the License.                                             //
//                                                                            //
// ========================================================================== //

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
