# Galaxy: an asynchronous distributed parallel visualization ray tracer
Galaxy is an asynchronous parallel visualization ray tracer for performant rendering in distributed computing environments. Galaxy builds upon [Intel OSPRay][1] and [Intel Embree][2], including ray queueing and sending logic inspired by [TACC GraviT][3]. 

Galaxy has been developed by Greg Abram and Paul Navratil with funding from the US Department of Energy's Office of Science ASCR program (Drs. Lucy Nowell and Laura Biven, program managers) through sub-award from Los Alamos National Laboratory; from Intel Corporation through Intel Parallel Computing Center awards; and from the Texas Advanced Computing Center at the University of Texas at Austin.

Galaxy is licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License. A copy of the License is included with this software in the file `LICENSE`. If your copy does not contain the License, you may obtain a copy of the License at: [Apache License Version 2.0][15]. 
Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.  

Copyright (c) 2014-2018 The University of Texas at Austin. All rights reserved.

## Galaxy dependencies
Galaxy has two types of dependencies: (1) components assumed to be already installed in an accessible spot; and (2) components that are associated with the Galaxy repository in the `third-party` subdirectory.

Galaxy assumes the following are already installed on your system in an accessible spot (these should all be available via package manager for your OS):
  * a modern MPI (Message Passing Interface) implementation
  * [a recent VTK][4] (at least version 7.x)
  * [a recent Qt][5] (for the Qt-based viewer app, at least version 5.x)
  * [Intel Thread Building Blocks (TBB)][6]
  * [PNG image format library][7]
  * [zlib compression library][8]
  * [CMake cross-platform build utility][9]
  * [GLFW][14] (required by OSPRay build)

Galaxy has the following components associated in the `third-party` subdirectory:
  * [Intel ISPC][10]
  * [Intel Embree][12]
  * [Intel OSPRay][13]
  * [rapidjson][16]

## Installing Galaxy associated dependencies
Prior to building Galaxy itself, you should ensure that all the general dependencies are installed (we recommend your humble OS package manager). Once those are in place, you're ready to install the dependencies in `third-party`. 

From the root directory of your local Galaxy repository, run the script `prep-third-party.sh`, which will init and update the git submodules, download ispc to where Galaxy expects to find it, and apply patches to the third-party CMake files to make them easier for Galaxy to find. This script will populate the `third-party/ispc`, `third-party/embree`, `third-party/ospray`, and `third-pary/rapidjson` subdirectories. 

All patch updates insert the prominent header: 
```
NOTE: This file has been modified by a Galaxy patch.
      See the GALAXY BEGIN / GALAXY END block(s) below.
```
and are bookended by prominent comment blocks of the format:
```
GALAXY BEGIN ADDED CODE - by Galaxy patch
< patch >
GALAXY END ADDED CODE - by Galaxy patch
```

If you prefer to use local installs of any of the dependencies, you can follow instructions below. Make sure to issue `git submodule init <path>` and `git submodule update <path>` for the third-party dependencies you do *NOT* have locally installed. For example, to use the rapidjson submodule, type:
```bash
git submodule init third-party/rapidjson
git submodule update third-party/rapidjson
```

### Installing ISPC
The `third-party/ispc` directory contains a script to download a recent ISPC binary for MacOS or Linux. ISPC installed using this script should be detected automatically by the Galaxy CMake configuration. If you already have a recent ISPC installed (at least version 1.9.1) you are free to use it, though you might need to specify its location by hand in the CMake configurations for Embree, OSPRay, and Galaxy.

From the root directory of your Galaxy repository, issue the following commands:
```bash
cd third-party/ispc
./get-ispc.sh 
```

This will download ISPC (currently version 1.9.2) into `third-party/ispc/ispc-v<version>-<OS type>` (e.g., ispc-v1.9.2-osx). If this binary does not work for you, you will need to build ISPC by hand following the directions at the [ISPC website][10]. 

### Installing Embree
After updating the git submodules as described above, the `third-party/embree` directory should contain the Embree source tree. We recommend building in `third-party/embree/build` and installing to `third-party/embree/install`, as doing so should enable OSPRay and Galaxy to find Embree automatically. The recommended install directory is configured as part of the Galaxy Embree patch.

First, apply the Galaxy Embree patch to the Embree repository. From the root directory of your Galaxy repository, issue the following commands:
```bash
cd third-party/embree
git apply ../patches/embree.patch
```

Now, create a build directory and build Embree. Assuming CMake can find all required dependencies, you can use the `cmake` command to configure the makefile for the Embree build:
```bash
mkdir build
cd build
cmake .. && make && make install
```
If cmake complains about missing dependencies, you can specify or change their locations using cmake `-D<CMAKE VAR>` command-line syntax or using the interactive `ccmake` interface with `ccmake ..` in the build directory.

### Installing OSPRay
Before installing OSPRay, make sure you have updated the Galaxy git submodules and successfully built Embree, as described above. Once the git submodules have been updated, the `third-party/ospray` directory should contain the OSPRay source tree. We recommend building in `third-party/ospray/build` and installing to `third-party/ospray/install`, as doing so should enable Galaxy to find OSPRay automatically. The recommended install directory is configured as part of the Galaxy OSPRay patch.

First, apply the Galaxy OSPRay patch to the OSPRay repository. From the root directory of your Galaxy repository, issue the following commands:
```bash
cd third-party/ospray
git apply ../patches/ospray.patch
```

Now, create a build directory and build OSPRay. Assuming CMake can find all required dependencies, you can use the `cmake` command to configure the makefile for the OSPRay build:
```bash
mkdir build
cd build
cmake .. && make && make install
```
If cmake complains about missing dependencies, you can specify or change their locations using cmake `-D<CMAKE VAR>` command-line syntax or using the interactive `ccmake` interface with `ccmake ..` in the build directory.

### Installing rapidjson
Before installing rapidjson, make sure you have updated the Galaxy git submodules as described above. Once the git submodules have been updated, the `third-party/rapidjson` directory should contain the rapidjson source tree. We recommend building in `third-party/rapidjson/build` and installing to `third-party/rapidjson/install`, as doing so should enable Galaxy to find rapidjson automatically. The recommended install directory is configured as part of the Galaxy rapidjson patch.

First, apply the Galaxy rapidjson patch to the rapidjson repository. From the root directory of your Galaxy repository, issue the following commands:
```bash
cd third-party/rapidjson
git apply ../patches/rapidjson.patch
```
Now, create a build directory and build rapidjson. Assuming CMake can find all required dependencies, you can use the `cmake` command to configure the makefile for the rapidjson build:

```bash
mkdir build
cd build
cmake .. && make && make install
```
If cmake complains about missing dependencies, you can specify or change their locations using cmake `-D<CMAKE VAR>` command-line syntax or using the interactive `ccmake` interface with `ccmake ..` in the build directory.

## Building Galaxy
Before building Galaxy, make sure all assumed and third-party subdirectory dependencies have been installed as described above, which will allow the Galaxy CMake configuration to find all dependencies automatically. We recommend building in `<Galaxy root>/build` and installing to `<Galaxy root>/install`. The recommended install directory is configured as part of the Galaxy CMake configuration.

Assuming CMake can find all required dependencies, you can use the `cmake` command to configure the makefile for the Galaxy build:

```bash
mkdir build
cd build
cmake .. && make && make install
```
If cmake complains about missing dependencies, you can specify or change their locations using cmake `-D<CMAKE VAR>` command-line syntax or using the interactive `ccmake` interface with `ccmake ..`.

## Running Galaxy

There are three ways to run Galaxy: 

* interactively using the Viewer
* in batch mode to produce a set of PNG image files
* in batch mode to produce a Cinema[17] image database

### Using the Viewer

So - lets give it a whirl.

```bash
mkdir $PROJECT/test
cd $PROJECT/test
```

Make some volumetric test data

`radial -r 256 256 256`

will create a .VTI dataset with several variables: ramps in X, Y and Z; oneBall is f(x, y, z) = mag(x, y, z) and eightBalls is f(x, y, z) = mag(abs(x) - 0.5, abs(y) - 0.5, abs(z) - 0.5)    We can create native format datasets:

`vti2raw radial-0.vti oneBall`

will create oneBall-0.vol and …raw.

Now we can use the Viewer.

`Viewer &`

In the dialog, select oneBall.vol and Open.   This will load the dataset into memory.   Note that this auto partitions the data - had I run 

`mpirun -np 4 Viewer &`

It would divide the data into 4 parts and assign one to each process.   Anyway, its in memory now; we can create a viewer:   in the menu bar, Viewers->New will pop up an empty viewer.  Now we attach the dataset to the viewer by hitting control-P on the viewer window to open its properties window.   Hitting Add will present you with a menu of the loaded datasets; doubleclick oneBall.

Still, nothing in the viewer, since we have yet to tell it how to visualize the data.   It should put up an outline, but it doesn’t.   Anyway, you’ll see oneBall in the viewer’s property window.   Select it and hit Edit to get the dataset’s properties.  In the Isovalues tab, hit Add to add an isovalue, and Commit.  Slide the widget and you should see a sphere of varying radius.  Similarly, Add in the Slices tab to add a slice and Commit.  Use of the Volume Rendering tab is left as an exercise for the user.

Now lets create a second dataset: back in the xterm, 

`vti2raw radial-0.vti eightBalls`

Then, select the viewer to get its menu bar, and File->Load Dataset  and load eightBalls.vol   You might need to change the file name extension in the dialog.   Now control-p in the viewer, and Add.   That’ll pop up a menu of available datasets - oneBall and eightBalls.   Doubleclick eightBalls and it will appear in the viewer’s property window; select it and Edit to get its properties, and you can add slices, isovalues, or volume rendering as above.  

Note the little ‘x’ button next to slice and isovalue interactors; it’ll delete the slice/isosurface.

By default the lighting is in object space; in the viewer’s dialog, select the Lights tab to change the lighting.   The little unlabelled checkbox will shift the light to be relative to the camera rather than the object.  You can also turn on shadows here, and change from a fixed ambient component to trace some AO rays.   32 is reasonable.  You can also change the radius of effect of the AO rays; here about 0.1 is reasonable.   Take a look, then turn it AO off again.  In the next section it’ll slow things down.

Now lets add some particles to the mix.   In the xterm, run:

`particles oneBall-0.vol -n 10000 -s PPP -r 0.05`

This will generate 10000 particles in the bounding box of the oneBall-0.vol dataset.   Unfortunately, particles datasets are not currently auto partitioned, so the PPP parameter specifies the number of partitions.   The -r parameter is a ghost zone width; particles within the ghost zone are duplicated in each partition so fragmentary particles don’t appear at boundaries.   The correct value would be the largest radius that particles will be rendered.     The particles.part output file contains the layout of the partitioned dataset (in XML format), indicating the bounding box and data file for each partition.

As before, add the particles.part data to the viewer.   By default, the radius is large; in the particles dataset properties, one parameter is available - the radius - and is given by a slider with bounds at 0 and 1.   Change the upper bound to 0.1 and move the slider down to get a reasonable result.  A value of 0.02 is reasonable.

### Batch Mode

Batch mode relies on a state file to define the one or more visualizations.   State files define a set of visualizations and a set of cameras.   A visualization consists of a specification of one or more datasets to appear in the visualization, along with the properties of each - the slicing planes, isovalues, transfer functions, radii etc.   The effect of the state file is the cross product of the sets of visualizations and cameras: a rendering of each visualization from every camera will be produced.

The following is a state file that should work with the datasets we created above, and will result in three renderings: a single visualization, containing two datasets, rendered from each of the three cameras.

```json
{
    "Datasets":
    [
      {
        "name": "oneBall",
        "type": "Volume",
        "filename": "oneBall-0.vol"
      },
      {
        "name": "eightBalls",
        "type": "Volume",
        "filename": "eightBalls-0.vol"
      }
    ],
    "Renderer": {
        "Lighting": {
          "Sources": [[0, 0, -4]],
          "shadows": true,
          "Ka": 0.2,
          "Kd": 0.8,
          "ao count": 0,
          "ao radius": 0.1
      }
    },
    "Visualizations":
    [
      {
        "annotation": "",
        "operators":
          [
            {
              "type": "Volume",
              "dataset": "eightBalls",
              "volume rendering": true,
              "isovalues": [ 0.05 ],
              "slices": [ [0, 0, 1, 0] ],
              "colormap": [
                [0.00,1.0,0.5,0.5],
                [0.25,0.5,1.0,0.5],
                [0.50,0.5,0.5,1.0],
                [0.75,1.0,1.0,0.5],
                [1.00,1.0,0.5,1.0]
              ],
              "opacitymap": [
              [ 0.00, 0.05],
              [ 0.20, 0.02],
              [ 0.21, 0.00],
              [ 1.00, 0.00]
             ]
            },
            {
              "type": "Volume",
              "dataset": "oneBall",
              "isovalues": [ 0.25 ],
              "volume rendering": false,
              "colormap": [
                [0.00,1.0,0.5,0.5],
                [0.25,0.5,1.0,0.5],
                [0.50,0.5,0.5,1.0],
                [0.75,1.0,1.0,0.5],
                [1.00,1.0,0.5,1.0]
               ],
              "opacitymap": [
                [ 0.00, 0.05],
                [ 0.20, 0.05],
                [ 0.21, 0.00],
                [ 1.00, 0.00]
               ]
            }
          ]
       }
    ],
    "Cameras":
    [
      {
        "viewpoint": [3, 0, -4],
        "viewcenter": [0, 0, 0],
        "viewup": [0, 1, 0],
        "aov": 30
      },
      {
        "viewpoint": [2, 3, -4],
        "viewcenter": [0, 0, 0],
        "viewup": [0, 1, 0],
        "aov": 30
      },
      {
        "viewpoint": [4, 3, -2],
        "viewcenter": [0, 0, 0],
        "viewup": [0, 1, 0],
        "aov": 30
      }
    ]
}
```

Considering each section in turn:

The Datasets section define the datasets that are available for visualization.   Each dataset specification includes the means to access the data, either using a file name or the information necessary to attach to an external source of data, and a name to be used internally.

The Renderer section includes the properties of the rendering, which are currently common among all the results of the run.   Rendering properties currently are simply the lighting model to be used, including the light sources themselves, whether to cast shadow rays or to add a fixed proportion of diffuse lighting, and whether to cast AO rays or to add a fixed proportion of ambient light.

The Visualizations section is an array, where each element (a visualization) contains an array of operators: one or more datasets and properties to be included in the visualization.   As an example, if the following is an element in a visualization operator array, that array will include the eightBalls dataset, with one slice, one isovalue and with volume rendering using the given transfer function.

```json
{
    "type": "Volume",
    "dataset": "eightBalls",
    "volume rendering": true,
    "slices": [ [0, 0, 1, 0] ],
    "isovalues": [ 0.05 ],
    "colormap": [
            [0.00,1.0,0.5,0.5],
            [0.25,0.5,1.0,0.5],
            [0.50,0.5,0.5,1.0],
            [0.75,1.0,1.0,0.5],
            [1.00,1.0,0.5,1.0]
           ],
    "opacitymap": [
            [ 0.00, 0.05],
            [ 0.20, 0.02],
            [ 0.21, 0.00],
            [ 1.00, 0.00]
           ]
}
```

Finally, the Cameras section is also an array, consisting of the cameras to be used.   Cameras are very simply specified.

If you cut’n’paste the complete state file above into a text file named batch.json in the test directory, you can run:

`[mpirun mpiargs] state [-s width height] batch.json`

You will produce three output .png files.   

### Cinema 

Cinema[17] databases are built in batch mode, and thus from a batch file that contains arrays of visualizations and cameras. This file may contain many visualizations; for example, if the the Cinema database has a varying isovalue and a moving slicing plane, there will be a separate visualization for each combination of isosurface value and slicing plane position.   Ten isovalues and ten slicing plane positions lead to 100 unique visualizations.   Similarly, the batch file may contain many cameras, one for each desired viewpoint. To make the creation of such batch files simple, the cinema.json file specifies a single visualization and camera parametrically.   

**Note: Galaxy currently creates spec C Cinema databases. You must download the Cinema viewer, the browser-based viewer requires spec D format**

The following shows an example cinema.json:
```json
{
    "Datasets":
    [
      {
        "name": "oneBall",
        "type": "Volume",
        "filename": "oneBall-0.vol"
      },
      {
        "name": "eightBalls",
        "type": "Volume",
        "filename": "eightBalls-0.vol"
      }
    ],
    "Renderer": {
        "Lighting": {
          "Sources": [[0, 0, -4]],
          "shadows": true,
          "Ka": 0.2,
          "Kd": 0.8,
          "ao count": 0,
          "ao radius": 0.1
      }
    },
    "Visualization":
    [
      {
        "type": "Isosurface",
        "dataset": "eightBalls",
        "isovalue range": { "min": 0.3, "max": 1.3, "count": 10 },
        "colormap": "eightBalls_cmap.json",
        "opacitymap": "eightBalls_omap.json"
      },
      {
        "type": "Slice",
        "dataset": "oneBall",
        "plane range":
        {
          "w": { "min": -0.9, "max": 0.9, "count": 10 },
          "normal": [0.0, 0.0, 1.0]
        },
        "colormap": "oneBall_cmap.json"
      },
      {
        "type": "Volume Rendering",
        "dataset": "eightBalls",
        "colormap": "eightBalls_cmap.json",
        "opacitymap": "eightBalls_omap.json"
       }
    ],
    "Camera":
    {
      "viewpoint": [3, 0, -4],
      "viewcenter": [0, 0, 0],
      "viewup": [0, 1, 0],
      "theta angle": { "min": -0.5, "max": 0.5, "count": 2 },
      "phi angle": { "min": -0.5, "max": 0.5, "count": 2 },
      "aov": 30
    }
}
```
In this example, we see a single “Visualization” element, rather than the array of visualizations given in a state file.   This, though, parametrically defines a set of 100
visualizations, consisting of the cross product of 10 isovalues and 10 offsets of a slicing plane (which varies from -1 to 1 based on the range of the intersection of the normal ray with the object bounding box.   Similarly, a single parametric camera is given as a base view defined by a viewpoint and view center, but varying the azimuth and elevation in a range (in radians).    This defines 4 cameras, and, with 100 visualizations, will create a Cinema database of 400 images.

Note, also, that the colormaps and opacity maps can be given as file names. 

Given such a cinema.json file, the included Python script cinema2state will expand the parametric cinema specification to an explicit batch file, and prepare a Cinema database.   If the above is in cinema.json, the corresponding Cinema database can be created by:

`cinema2state cinema.json`

which will create state.json and initialize cinema.cdb.  Following this, 

`[mpirun mpiargs] state [-s width height] -C state.json`

will render the necessary images and deposit them into cinema.cdb.

## Galaxy Notes

## Performance and a note on threading

This renderer is parallel both by using multiple processes over MPI and by threading within each process.  Both the generation of initial rays and the processing of rays is now multithreaded.   By default, only a single thread is used, but the GXY_NTHREADS environment variable can be used to increase the number of worker threads.   

On my Mac Powerbook (2.8 GHz 4 core), the above Cinema database requires 579 seconds to render at 500x500 resolution.   Using GXY_NTHREADS=8 (the activity monitor indicates there are 8 virtual cores - hyperthreading?) reduces rendering time to 156 seconds.

### Galaxy environment variables
The following environment variables affect Galaxy behavior:

  * **GXY_NTHREADS** : use the requested number of threads in the rendering thread pool (default 1)
  * **GXY_APP_NTHREADS** : use the requested number of threads for the application (default *TBB default*)
  * **GXY_FULLWINDOW** : render using the full window
  * **GXY_RAYS_PER_PACKET** : The number of rays to include in a transmission packet (default 10000000)
  * **GXY_RAYDEBUG** : turn on ray debug pathway, taking **GXY_X**, **GXY_Y**, **GXY_XMIN**, **GXY_XMAX**, **GXY_YMIN**, **GXY_YMAX** from environment variables
  * **GXY_X** : x coordinate for single-ray debug (requires **GXY_RAYDEBUG**)
  * **GXY_Y** : y coordinate for single-ray debug (requires **GXY_RAYDEBUG**)
  * **GXY_XMIN** : lower x extent for debug window (requires **GXY_RAYDEBUG**)
  * **GXY_XMAX** : upper x extent for debug window (requires **GXY_RAYDEBUG**)
  * **GXY_YMIN** : lower y extent for debug window (requires **GXY_RAYDEBUG**)
  * **GXY_YMAX** : upper y extent for debug window (requires **GXY_RAYDEBUG**)


[1]: http://www.ospray.org/
[2]: http://embree.github.io/
[3]: http://tacc.github.io/GraviT/
[4]: http://www.vtk.org/
[5]: http://www.qt.io/
[6]: https://software.intel.com/en-us/intel-tbb
[7]: http://libpng.org/pub/png/libpng.html
[8]: http://zlib.net/
[9]: http://www.cmake.org/
[10]: https://ispc.github.io/
[12]: http://embree.github.io/
[13]: http://www.ospray.org/
[14]: http://www.glfw.org/
[15]: https://www.apache.org/licenses/LICENSE-2.0
[16]: http://rapidjson.org/
[17]: https://cinemascience.org/

