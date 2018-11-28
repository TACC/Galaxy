# Galaxy Configuration Files

Galaxy uses JSON files to represent its configuration state. State files are currently used to initialize Galaxy when rendering, both for the interactive viewer and for creating [Cinema][1] image databases. Galaxy also supports a special `.cinema` JSON specification for creating a state file for Cinema use.

## Galaxy State Files
### Supported Data Types

Galaxy accepts the following data formats:

* "brick of floats"
* Parallel VTK formats (vtu, ptv)
* "ptri" file – bounding box of original volume from which isosurface was extracted, followed by isosurface extraction file

*NOTE:* Currently, bounding boxes must match (count and extent) for each dataset included in the renderer. 

### Supported Rendering Options

* Rendering
    - Ray tracing surfaces
    - Volume rendering
* Visualization Filters
    - Isosurface
    - Clip
    - Slice
    - Sphere particle modeling
* Lighting options 
    - Phong
    - Cross-node ambient occlusion

### State File Format

A Galaxy state file describes all aspects required to produce a set of images that visualize given data. The state file contains four sections:

* Datasets
    - Name, type, filename
    - Type: \[ Volume, Particles, Triangles \]
* Renderer
    - Lighting: sources, shadows, Ka, Kd, AO count, AO radius
* Visualizations
    - Type: \[ Volume, Particles, Triangles \]
    - Operators: isovalues, slices
    - Volume rendering
    - Transfer function: \[ color, opacity \]
* Cameras
    - Location, up, at, angle of view

Galaxy will create the cross product of Visualizations x Cameras in a Cinema type A database. There is a script in the Galaxy `scripts` directory to transform a Cinema type A database into a Cinema type D database for use with the browser-based Cinema explorer.

The top-level Galaxy JSON format for state files is below, followed by the specifcation for each section. A single `Visualization` or `Camera` can be specified without array notation. When Galaxy is run in interactive mode, the first `Camera` is used as the initial viewpoint.  

Statefile sketch
```json
{
    "Datasets": [ dataset spec ], 
    "Renderer": render spec, 
    "Visualizations": [ visualization spec ], 
    "Cameras": [ camera spec ]
}
```

Datasets spec example (note the array notation):
```json
"Datasets" :
[
    {
        "name": "oneBall",
        "filename": "/home/gda/data/radial/oneBall.vol", 
        "type": "Volume"
    }, 
    {
        "name": "eightBalls",
        "filename": "/home/gda/data/radial/eightBalls.vol", 
        "type": "Volume"
    } 
]
```

Render spec example:
```json
"Renderer" :
{
    "Lighting": {
        "Sources": [ light spec ], 
        "shadows": false,
        "Ka": 0.4,
        "Kd": 0.6,
        "ao count": 0,
        "ao radius": 0.5 
    }
}
```

Galaxy light spec is in the format `\[ x, y, z, type \]` where `x, y, z` specifies a point or vector, depending on light type, and where `type` specifies an optional light type: 

* `0` - point light, `x, y, z` specifies location (default if type is not specified)
* `1` - directional light, `x, y, z` specifies direction (infinitely far from data)
* `2` - camera light, `x, y, z` specifies displacement from camera point

Visualization spec example:
```json
"Visualizations" : 
{
    "annotation": "my awesome visualization"
    "operators": [
        {
            "type" : "Volume",
            "dataset": "oneBall",
            "colormap": [
                [ 0.0, 1.0, 0.5, 0,5],
                [ 0.5, 0.5, 1.0, 0.5], 
                [ 1.0, 0.5, 0.5, 0.5]
            ], 
            "opacitymap": [
                [ 0.0, 0.0 ],
                [ 1.0, 1.0 ] 
            ],
            "isovalues": [ comma separated list of isovalues ], 
            "slices": [ [ 4 floats defining a plane ], ... ], 
            "volume rendering": false
        }
    ]
}
```

The format of colormaps and opacitymaps are as follows:

* colormap – \[ d, r, g, b \]
* opacitymap – \[ d, o \] 
 
where `d` represents normalized data values, `r, g, b` represent RGB color, and `o` represents opacity. The reader makes the following assumptions:

* All value ranges between \[0, 1\] (i.e. normalized)
* Values in monotonically increasing order 
* Opacity map only used for Volume Rendering


Camera spec example:
```json
{
    "aov": 60, 
    "viewpoint": [0, 0, 5], 
    "viewcenter": [0, 0, 0], 
    "viewup": [0, 1, 0]
}
```
The `aov` (angle of view) is interpreted in degrees.


## Galaxy Cinema Files

Galaxy also supports a `cinema` JSON format that allows users to specify a range of `Camera` postions and `Visualization` operations that can then be converted into a Galaxy state file using the `cinema2state` script. The cinema JSON format supports the following manipulations:

* Camera orbits
* Isosurface sweeps
* Clipping planes
* Transfer function modulation

For camera position manipulations, the format adds `theta angle` and `phi angle` JSON elements to describe the camera orbit around the camera's `viewcenter`. The `min` and `max` parameters are evaluated in radians. These will produce `theta angle count` x `phi angle count` separate cameras in a Galaxy state file.

Cinema Camera spec example:
```json
{
    "viewpoint": [30, 30, 30], 
    "viewcenter": [0, 0, 0], 
    "viewup": [0, -1, 0], 
    "theta angle": {
        "min": -3.1415, 
        "max": 3.1415, 
        "count": 100
    }, 
    "phi angle": {
        "min": -0.10, 
        "max": 0.10, 
        "count": 10
    },
    "aov": 30 
}
```

Similarly, the user can specify `isovalue range` and `plane range` elements in a visualization's operator specification. Note that colormaps and opacitymaps can be specified in separate files.

```json
{
    "annotation": "my awesome visualization"
    "operators": [
        {
            "type" : "Isosurface",
            "dataset": "eightBalls",
            "isovalue range": { "min": 0.3, "max": 1.3, "count": 3 },
            "colormap": "eightBalls_cmap.json",
            "opacitymap": "eightBalls_omap.json",
        }, 
        {
            "type" : "Slice",
            "dataset": "oneBall",
            "plane range":
            {
                "w": { "min": -0.9, "max": 0.9, "count":10 },
                "normal": [0.0, 0.0, 1.0]
            },
            "colormap": "oneBall_cmap.json"
        }
    ]
}
```


[1]: https://www.cinemascience.org/