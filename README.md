# PVOL parallel visualization ray tracer
PVOL is a parallel visualization ray tracer for performant rendering in distributed computing environments. PVOL builds upon [Intel OSPRay][1] and [Intel Embree][2], including ray queueing and sending logic inspired by [TACC GraviT][3]. PVOL has been developed by Greg Abram with funding from the US Department of Energy through Los Alamos National Laboratory; from Intel Corporation through the Intel Parallel Computing Center program; and from the Texas Advanced Computing Center at the University of Texas at Austin.

## PVOL dependencies
PVOL has two types of dependencies: (1) components assumed to be already installed in an accessible spot; and (2) components that are associated with the PVOL repository in the `third-party` subdirectory.

PVOL assumes the following are already installed on your system in an accessible spot (these should all be available via package manager for your OS):
  * a modern MPI (Message Passing Interface) implementation
  * [a recent VTK][4] (at least version 7.x)
  * [a recent Qt][5] (for the Qt-based viewer app)
  * [Intel Thread Building Blocks (TBB)][6]
  * [PNG image format library][7]
  * [zlib compression library][8]
  * [CMake cross-platform build utility][9]
  * [GLFW][14] (required by OSPRay build)

PVOL has the following components associated in the `third-party` subdirectory:
  * [Intel ISPC][10]
  * [threadpool11][11]
  * [Intel Embree][12]
  * [Intel OSPRay][13]

## Installing PVOL associated dependencies
Prior to building PVOL itself, you should ensure that all the general dependencies are installed (we recommend your humble OS package manager). Once those are in place, you're ready to install the dependencies in `third-party`. 

From the root directory of your local pvol repository, issue the following git commands:
```
git submodule init
git submodule update
```

These commands will populate the `third-party/threadpool11`, `third-party/embree` and `third-party/ospray` subdirectories. 

### Installing ISPC
The `third-party/ispc` directory contains a script to download a recent ISPC binary for MacOS or Linux. ISPC installed using this script should be detected automatically by the PVOL CMake configuration. If you already have a recent ISPC installed (at least version 1.9.1) you are free to use it, though you might need to specify its location by hand in the CMake configurations for Embree, OSPRay, and PVOL.

From the root directory of your PVOL repository, issue the following commands:
```
cd third-party/ispc
./get-ispc.sh <OS type>     # where <OS type> = [ linux | osx ]
  
```

This will download ISPC (currently version 1.9.2) into `third-party/ispc/ispc-v<version>-<OS type>` (e.g., ispc-v1.9.2-osx). If this binary does not work for you, you will need to build ISPC by hand following the directions at the [ISPC website][10]. 

### Installing Embree
After updating the git submodules as described above, the `third-party/embree` directory should contain the Embree source tree. We recommend building in `third-party/embree/build` and installing to `third-party/embree/install`, as doing so should enable OSPRay and PVOL to find Embree automatically. The recommended install directory is configured as part of the PVOL Embree patch.

First, apply the PVOL Embree patch to the Embree repository. From the root directory of your PVOL repository, issue the following commands:
```
cd third-party/embree
git apply ../../patches/embree.patch
```

Now, create a build directory and build Embree. Assuming CMake can find all required dependencies, you can use the `cmake` command to configure the makefile for the Embree build:
```
mkdir build
cd build
cmake .. && make && make install
```
If cmake complains about missing dependencies, you can specify or change their locations using cmake `-D<CMAKE VAR>` command-line syntax or using the interactive `ccmake` interface with `ccmake ..`.

### Installing OSPRay
Before installing OSPRay, make sure you have updated the PVOL git submodules and successfully built Embree, as described above. Once the git submodules have been updated, the `third-party/ospray` directory should contain the OSPRay source tree. We recommend building in `third-party/ospray/build` and installing to `third-party/ospray/install`, as doing so should enable PVOL to find OSPRay automatically. The recommended install directory is configured as part of the PVOL OSPRay patch.

First, apply the PVOL OSPRay patch to the OSPRay repository. From the root directory of your PVOL repository, issue the following commands:
```
cd third-party/ospray
git apply ../../patches/ospray.patch
```

Now, create a build directory and build OSPRay. Assuming CMake can find all required dependencies, you can use the `cmake` command to configure the makefile for the OSPRay build:
```
mkdir build
cd build
cmake .. && make && make install
```
If cmake complains about missing dependencies, you can specify or change their locations using cmake `-D<CMAKE VAR>` command-line syntax or using the interactive `ccmake` interface with `ccmake ..`.

### Installing threadpool11
Before installing threadpool11, make sure you have updated the PVOL git submodules as described above. One the git submodules have been updated, the `third-party/threadpool11` directory should contain the threadpool11 source tree. We recommend building in `third-party/threadpool11/build` and installing to `third-party/threadpool11/install`, as doing so should enable PVOL to find threadpool11 automatically. The recommended install directory is configured as part of the PVOL threadpool11 patch.

First, apply the PVOL threadpool11 patch to the threadpool11 repository. From the root directory of your PVOL repository, issue the following commands:
```
cd third-party/threadpool11
git apply ../../patches/threadpool11.patch
```

Now, create a build directory and build threadpool11. Assuming CMake can find all required dependencies, you can use the `cmake` command to configure the makefile for the threadpool11 build:
```
mkdir build
cd build
cmake .. && make && make install
```
If cmake complains about missing dependencies, you can specify or change their locations using cmake `-D<CMAKE VAR>` command-line syntax or using the interactive `ccmake` interface with `ccmake ..`.

## Building PVOL
Before building PVOL, make sure all assumed and third-party subdirectory dependencies have been installed as described above, which will allow the PVOL CMake configuration to find all dependencies automatically. We recommend building in `<PVOL root>/build` and installing to `<PVOL root>/install`. The recommended install directory is configured as part of the PVOL CMake configuration.

Assuming CMake can find all required dependencies, you can use the `cmake` command to configure the makefile for the threadpool11 build:
```
mkdir build
cd build
cmake .. && make && make install
```
If cmake complains about missing dependencies, you can specify or change their locations using cmake `-D<CMAKE VAR>` command-line syntax or using the interactive `ccmake` interface with `ccmake ..`.

## Running PVOL


### PVOL environment variables
The following environment variables affect PVOL behavior:
  * **FULLWINDOW** : render using the full window
  * **RAYS_PER_PACKET** : The number of rays to include in a transmission packet (default 10000000)
  * **RAYDEBUG** : turn on ray debug pathway, taking **X**, **Y**, **XMIN**, **XMAX**, **YMIN**, **YMAX** from environment variables
  * **X** : x coordinate for single-ray debug (requires **RAYDEBUG**)
  * **Y** : y coordinate for single-ray debug (requires **RAYDEBUG**)
  * **XMIN** : lower x extent for debug window (requires **RAYDEBUG**)
  * **XMAX** : upper x extent for debug window (requires **RAYDEBUG**)
  * **YMIN** : lower y extent for debug window (requires **RAYDEBUG**)
  * **YMAX** : upper y extent for debug window (requires **RAYDEBUG**)


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
[11]: https://github.com/tghosgor/threadpool11
[12]: http://embree.github.io/
[13]: http://www.ospray.org/
[14]: http://www.glfw.org/