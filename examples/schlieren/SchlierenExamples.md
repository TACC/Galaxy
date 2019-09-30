# Schlieren Examples

This example demonstrates the use of Galaxy's Schlieren rendering capabilities.  As described in the source directory, two algorithms have been implements: one in which the each pixel is assigned a value reflecting the deflection of the ray originally spawned for the pixel, and one in which the delflected ray contributes energy to the image plane wherever it arrives.

In the first case, the *x*, *y* and *z* deflections are represented in the *r*, *g* and *b* FITS files, while the *a* file contains the magnitude of the displacement.

In the second case, only the *r* FITS file contains meaningful data.

Note that these are *floating-point* images; to convert them to color images, use (e.g.) **ColorMoves**.

To run the first algorithm:

**gxyschlieren [-S1] csafe-1.state**

To run the second:

**gxyschlieren -S2 csafe-s.state**

In each case, three sets of results are created, showing projections along the X, Y and Z axis.