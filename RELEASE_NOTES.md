# Galaxy Release Notes

## version 0.5.0 - pending

* *TODO* - centralize data filtering codes currently in `src` to `src/filters`
* *TODO* - scripts to launch singularity-based multi-node distributed testing on TACC systems

## version 0.4.0 - 20200630
* Added `gui` and `guiserver`
  - a Qt-based node editor[4] graphical user interface for Galaxy workflows
  - added `nodeeditor` submodule
  - added Qt5 dependency
* Added `insitu` socket connectivity for in situ workflows
* Added `tracer` streamline generation and rendering
* Added `sampler` methods
  - Gradient-based sampling
  - Isovalue-based sampling
* Added demonstration codes for new features in `apps` 
* Updated to python3
  - python scripts
  - travis-ci config
  - travis-ci VTK-8.1 build

## version 0.3.2 - 20191212

* updated OSPRay version to 1.8.5
* updated Embree version to 3.6.1
* updated ISPC version to 1.12.0
* centralized third-party installation to third-party/install directory
* removed patchfiles for third-party packages, moved config options to cmake arguments in `scripts/install-third-party.sh`

## version 0.3.1 - 20190930

* added examples for Schlieren renderer in `examples/schlieren`
* added examples for various data samplers in `examples/sampletrace`
* added second, energy-accumulation schlieren algorithm
* added 'data.state' argument to `sampletrace` to track data
* moved image resolution specification to Camera
* added Cinema database example of Water in the Universe data in `examples/WitU`
* added 'data range' to visualizations to select range of interest

## version 0.3.0 - 20190821

* Added `sampler` with a variety of data-space sampling algorithms:
  - metropolis-hastings sampling as a multiserver tool 
  - ray-based sampling using the 'Sampler' infrastructure
    + Sampler is a subclass of Renderer that supports integration with active-Vis operators
    + active-Vis operators to implement sampling based on isosurface crossing and significant changes to gradient
  - interpolation of a scalar field onto a set of samples (as a multiserver tool)
* Support for vector-valued volumes
  - added JSON format for volume description files that implements 'number of components' field
* Initial implementation of RungeKutta operator to trace streamlines in vector volumes
  - also 'TraceToPathLines' operator to convert RungeKutta results to path lines for rendering
* Added `schlieren` to provide multi-hued Schlieren and shadowgraph-style rendering
  - also implemented 'float-image' output (in FITS format)
* Added `ospray` modules for class overrides to the underlying [OSPRay][1] rendering engine
* Separated data-generation and data-management tools into `data` from `apps`
* Added Docker containers:
    - [`galaxy`][2]: contains pre-built version of galaxy in CentOS linux
    - [`galaxy-base`][3]: contains pre-requisites for galaxy build in CentOS linux
* Added unit testing framework
* Renamed `async` app and associated source to `gxyviewer`
* Renamed `vis` app and associated source to `gxywriter`
* Renamed `mh` app and associated source to `mhwriter` 
* Renamed `amh` app and associated source to `mhviewer`
* updated third-party/ispc to download v1.10.0
* updated third-party/embree to use v3.5.2
* updated third-party/ospray to use v1.7.3

## version 0.2.1 - 20190325

* Moved Doxygen to root Galaxy directory
* Fixed includes for Fedora 29 build

## version 0.2.0 - 20190208

Refactorization of Galaxy core to cleanly partition framework, data, and renderer layers. Initial release of multiserver modularization of Galaxy framework.

* Added new `multiserver` component that enables Galaxy to live as a service (e.g. alongside a simulation *in situ*) and process requests that arrive via remote connections.
* Removed OSPRay and Embree dependencies from framework and data layers.
* Install now generates a GalaxyConfig.cmake file so that Galaxy components can be included in other programs
* Added Travis-CI build and test integration
* Added sanity-check gold image tests in `tests`
* Added Galaxy state file documentation in `doc/configuration_files.md`
* Added Python as a prerequisite for testing and use of python scripts in `scripts`

## version 0.1.0 - 20181005

Initial Galaxy release!

* Asynchronous tasking and communication framework
* Data layer for visualization and rendering
* Distributed ray tracing renderer built on Intel OSPRay and Embree for volume integration and geometry intersection, respectively
* Doxygen-based documentation of classes
* CMake build and packaging


[1]: https://ospray.org/
[2]: https://hub.docker.com/r/pnav/galaxy
[3]: https://hub.docker.com/r/pnav/galaxy-base
[4]: https://github.com/paceholder/nodeeditor


