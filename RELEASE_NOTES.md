# Galaxy Release Notes

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


