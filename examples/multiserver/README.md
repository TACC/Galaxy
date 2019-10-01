# MultiServer Demo Instructions 
## Preparing Environment
Build the code using **-DGXY_WRITE_IMAGES=OFF** in the cmake step

I’ll assume that you have the code built and installed in $INSTALL.  First we set up the environment

```
source $INSTALL/galaxy.env
```
Preparing Data


In a terminal window, make a test directory, enter it and create the input data:

```
radial -r 128 128 128
vti2vol vti2vol radial-0.vti oneBall eightBalls
```

Depending on your setup, you may need to run vti2vol under vtkpython..

```
vti2vol $INSTALL/bin/vti2vol radial-0.vti oneBall eightBalls
```

## Running the server

Assuming that you are using full paths when you run the msdata client, you can execute this anywhere, but the test directory might be a good place:

```
mpirun -np NUM msserver [-P port]
```

where NUM is the number of parallel processes to run (4 is a good number for a laptop) the and port defaults to 5001.

Type '**quit;**' or control-D at the prompt to kill the server.

## Loading the data

```msdata [-H hostname] [-P port] [file.state ... file.state] [-]```
or
```msclient -so libgxy_module_data.so [-H hostname] [-P port] [file.state ... file.state] [-]```

The hostname defaults to **localhost** and the port to **5001**. If you give it one or more state file names, it pulls commands from them in order. If you leave a hyphen at the end, it waits at a command prompt; otherwise it simply exits.

Commands at the prompt include:

* import json;
 
where json is a JSON string describing a Datasets section from Galaxy state files; eg.

```
  {
    "Datasets": 
    [
      {
		  "name": "oneBall",
		  "type": "Volume",
		  "filename": "/Users/gda/galaxy/test/radial/data/radial-0-oneBall.vol"
	   }  
	 ]
  }
```

Note that this JSON string is exactly what would otherwise be in a state file.

More than one dataset can be included; don’t forget the trailing semicolon:

```
import {
			"Datasets": 	
			[
				{
				"name": "oneBall",
				"type": "Volume",
				"filename": "/Users/gda/galaxy/test/radial/data/radial-0-oneBall.vol"
				}, 
				{
				"name": "eightBalls",
				"type": "Volume",
				"filename": "/Users/gda/galaxy/test/radial/data/radial-0-eightBalls.vol"
				} 
		    ]
		};
```

* list;
		
Lists out loaded datasets; after the irst command above, will reply ‘oneBall’.  After the second it will replyt with both 'oneBall' and 'eightBalls'.

* file file.in;
	
Process the commands in file.in.  Anything you could type at the commmand line can be included.

* quit;

Note that file and quit are supported in all msclient-barased clients.
## Sampling the data

```mssample [-H hostname] [-P port] [file ... file] [-]```
or
```msclient -so libgxy_module_mhsampler.so [-H hostname] [-P port] [file ... file] [-]```


The hostname defaults to localhost and the port to 5001. If you give it one or more file names, it pulls commands from them in order. If you leave a hyphen at the end, it waits at a command prompt after it processes any files; otherwise it simply exits.

In addition to file and quit, several command-line commands include:

* volume volume;

name of volume object to sample (required)

* particles partices; 

name of particles object to create or modify (required)

* iterations n; 

number of iterations, 20000 default

* tf-{type} arg0 arg1;

type of transfer function, type is gaussian or linear
and arg0 and arg1 are parameters to the function Defaults to tf-linear {datamin} {datamax} which identifies the data max as the most important and the data min as the least.

* skip n;

hot-start count, defaults to 10

* miss n; 

quit after n successive samples miss. default 10

* color r g b a; 

constant color assigned to samples, default 0.8 0.8 0.8 1.0

* radius r; 

Constant radius; default 0.02

* sample;

run the algorithm


The following generates samples toward the outside of the oneBall data since it maps the highest priority to 1.7, which is the values at the extreme corners of the dataset and 1.0 (and below) to a priority of 0.

```
volume oneBall; 
particles gsamples; 
iterations 10000; 
tf-linear 1.0 1.7; 
color 0.5 1.0 0.5 1.0; 
radius 0.03;
sample;
```

##Viewing the data
Once data is loaded, run:

```msviewer [-H hostname] [-P port] [-s w h] [-a age fadeout] statefile```

Where:

* -s w h 

width and height of the window (500x500)

* -a start finish

fadeout age at which to start fading non-updated pixels and time
over which the pixels will be faded out – defaults to 3 1 which is way too long; 0.2 0.2 is better.

And the state file is as before, except it does not include the Datasets component. The following will show the two samplings of the oneBall datasets and a slice of the eightBalls Dataset, with shadows and ambient occlusion.
Note: rendering doesn’t begin until you actually mouse-down and drag!

```
	{
	    "Cameras": 
		    [
				{
					"annotation": "",
					"aov": 35,
					"viewcenter": [ 0, 0, 0 ], "viewpoint": [ 1.0, 2.0, 4.0], "viewup": [ 0.0, 1.0, 0.0 ]
				} 
			 ],
		 "Renderer": 
			 {		
			 },
		 "Visualization":
			 {
				"annotation": "", 
				"Lighting": 
					{
						"Ka": 0.5,
						"Kd": 0.5,
						"Sources": [ [ 1, 2, 0, 1 ] ], 
						"ao count": 16,
						"ao radius": 1.0, 	
						ls -lt"shadows": true
					},
			   "operators": 
			   		[
						{
							"dataset": "rsamples", "type": "Particles"
						}, 	
						{
							"dataset": "rsamples",
							"type": "Particles" 
						},
						{
							"dataset": "eightBalls", 
							"type": "Volume",
							"colormap": 
								[
									[ 0.0, 1.0, 0.5, 0.5 ],
									[ 0.25, 0.5, 1.0, 0.5 ], 
									[ 0.5, 0.5, 0.5, 1.0 ],
									[ 0.75, 1.0, 1.0, 0.5 ], 									[ 1.0, 1.0, 0.5, 1.0 ]
								],
							"opacitymap": 
							[
								[ 0.0, 0.05 ], 
								[ 0.4, 0.02 ], 
								[ 0.41, 0.0 ],
								[ 1.0, 0.0 ]
							],
							"plane": [ 1.0, 0.0, 0.0, 0.0 ],
							"volume rendering": true
						}
					]
			 }
		}
	}
```

The resulting image follows. 

Note that the holes in the spheres are due to the spheres crossing the limits of the data bounding box.

![Image of example]
(
example.png)