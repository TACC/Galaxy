# Schlieren 

The files in this directory contain two implementations of Schlieren rendering.  Each is implemented as subclasses of *Renderer* which replace the Trace method to do bendy schlieren integration rather than the straight-ray tracing done by the standard Renderer.   

The examples can be run in the 

## Schlieren

Trace initially orthographic rays through a diffractive volume according to Snell's law.  Results at each pixel reflect the vector in the projection plane between the pixel center and where the ray *originally destined* for that point ended up.   The r result is deflection in X, the g is the deflection in Y, the b is the deflection in Z (the projection plane is arbitrary).  The a result is the magnitude of the deflection vector.

Example:

**gxyschlieren csafe-1.state**

Generates four FITS files.  Note that these are 'floating point' images; use eg. *Colormoves* to create a normal colored image.

## Schlieren2

Trace initially orthographic rays through a diffractive volume according to Snell's law.  Rays striking the projection plane are accumulated at the hit point.   Only the first FITS file contains meaningful data

This algorithm also implements its own subclass of *Rendering* which accumulates hits on the image plane.

Example:

**gxyschlieren -S2 csafe-2.state**

Again, this result is a 'floating point' image; use eg. *Colormoves* to create a normal colored image.

