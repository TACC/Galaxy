#ksndr

ksndr is a parallel process that simulates a simuator.   It either generates random data or sends data from a list of input files.   It can be run multi-process under MPI.   You can tell how many senders to use (overwritten by the length of the list of input files, if given).   It'll use threads if there aren't enough parallel processes.  If you don't tell it how many senders and there's no input files, it uses the number of parallel processes.

First timestep data is created or loaded immediately.   It then waits for input; a 's' causes it to send a timestep; a 'c' causes it to create a new timestep of random data; a 'S' does both.   'q' to quit.

	syntax: (mpi) ksndr [options]
	options:
		-p pfile		-- list of data files; can be vtp, vtu, csv, raw
						csv:
							n
							x, y, z, d
							...
						raw: binary version of above
		
		-b box      	-- a file containing six space separated values: xmin xmax ymin ymax zmin zmax (-1 1 -1 1 -1 1) 
	
		-h hostfile 	-- a file containing a host name for each receiver process (localhost)
						duplicate to create >1 process / node
  	
		-p npart     	-- number of particles to send to each receiver (100)
						in the case of randomly generated data
   
		-n nproc    	-- number of senders to run (1)
						if pfile is given, this is ignored and the number of data files is used
	
		-f fuzz     	-- max radius of points - creates empty border region (0)
	
	
#kwrtr

kwrtr is the receiver and renderer.  Given a state file indicating how to render images, it waits for timestep data from a sender side, then redistributes the data into partitions as defined by the -b option and the number of processes.   It then renders it as per the state file.   Each timestep produces a set of image files as defined in the state file.  It then waits for a new timestep.   'q' at the prompt to quit.

ksndr can be run in any number of parallel processes. If there are more senders than receiver processes, some or all receivers will receive data from more than one senders.  If there are more receivers than senders, some won't expect data.  

	syntax: (mpi) kwrtr [options] state
	options:
		-f fuzz    sort-of ghost zone width (0.0)

		-n nsndrs  number of external processes sending particle data (1)

		-C cdb     put output in Cinema DB (no)
		
		-s w h     image width, height (1920 1080)
		
		-N         max number of simultaneous renderings (VERY large)


## note:
ksndr expects kwrtr processes to have ports open and ready for input

#Minimal example:

##krishna.state
```
{
    "Cameras": [
        {
            "annotation": "",
            "aov": 35,
            "viewcenter": [ 0, 0, 0 ],
            "viewpoint": [ 3.0, 4.0, 3.0 ],
            "viewup": [ 0.0, 0.0, 1.0 ]
        }
    ],
    "Datasets": [
        {
            "filename": "particles.part",
            "name": "particles",
            "type": "Particles"
        }
    ],
    "Renderer": {
    	"box": [-1, 1, -1, 1, -1, 1],
    	"epsilon": 0.0001
    },
    "Visualizations": [
        {
            "annotation": "",
            "Lighting": {
                "Ka": 0.5,
                "Kd": 0.5,
                "Sources": [ [ -1, -2, -4 ] ],
                "ao count": 0,
                "ao radius": 1.0,
                "shadows": true
            },
            "operators": [
                {
                  "dataset": "particles",
                  "type": "Particles",
                  "colormap": [ [0, 1.0, 1.0, 0.5], [1, 0.5, 0.5, 1.0]],
                  "radius0": 0.05,
                  "radius1": 0.1,
                  "value0": 0,
                  "value1": 1,
                }
            ]
        }
    ]
}
```
##Single processes on each side

In one window: ```kwrtr krishna.state```
<br>
In another:  ```ksndr -f 0.1 [-h hostfile]``` followed by an 's' at the prompt.


One <b>ksndr</b> task will send a 'timestep' of ~100 points in the default box (+/- 1, +/- 1, +/- 1) and a data value of zero (the sender ID divided by the total number of senders, if not zero) to one <b>kwrtr</b> task, which will render one image as per <b>krishna.state</b>.   Since the data is constant, the spheres will take on a size of 0.1.  

We can give the sender the -h hostfile argument to tell it where to send the data.   By default, its sent to 'localhost', so this isn't necesssary here.   

We give ```ksndr``` the -f 0.1 option to prevent it from conjuring points within 0.1 of the global boundary and being cut off.   This could also be fixed by expanding the box in the state file to +/- 1.1 so that points on the +/- 1 boundary don't get cut off.

##Multiple renderers
							
In one window: ```(mpirun) kwrtr krishna.state -f 0.1```
<br>
In another:  ```ksndr [-h hostfile]``` followed by an 's' at the prompt.

One <b>ksndr</b> task will send a 'timestep' of ~100 points in the default box (+/- 1, +/- 1, +/- 1) and a data value of zero (the sender ID divided by the total number of senders, if not zero) to however many <b>kwrtr</b> tasks are started by mpirun.  One image will be rendered as per <b>krishna.state</b>.   Since the data is constant, the spheres will take on a size of 0.1.

Now we have more than one renderer, each responsible for a partition of the data space.  We therefore need to reshuffle the incoming points so that each process has access to the points that affect its partition of space.    Since the max sphere radius is 0.1, we add the <b>-f 0.1</b> option so that the data is distributed with the necessary overlap.

Again, we can give a host file to tell the sender where the renderers are.   However, it defaults so that all renderers are on 'localhost'.   Since there's a single sender, it'll send to 'localhost'.

	
##Multiple senders

In one window: ```kwrtr krishna.state -f 0.1 ```
<br>
In another:  ```(mpirun)  ksndr [-h hostfile] -n K``` followed by an 's' at the prompt.


Now K senders (determined by <b>-n K</b> argument) will send data to one renderer.   The K senders can be spread across any number of processes (less than or equal to K) which are started by mpirun.  Since there's only one renderer, the default behavoir will be for all senders to send to a single recipient at localhost.
	
##Multiple senders and renderers

In one window: ```(mpirun) kwrtr krishna.state -f 0.1 -n nsenders```
<br>
In another:  ```(mpirun)  ksndr -h hostfile -n K``` followed by an 's' at the prompt.


Now K senders (determined by -n argument) will send data to multiple renderers, as per the mpirun arguments passed when starting the renderers.   As before, the K senders can be spread across any number of processes (less than or equal to K) which are started by mpirun.  In this case, however, the senders send to multiple renderers, so they need to know how many and where they are.   We therefore need to specify the <b>-h hostfile</b> argument.  The K senders will send to the listed renderers, wrapping the list where necessary.   Note that there is an implicit port number associated with each line of the hostfile - the n'th renderer will be listening at port 1900+n on the n'th host listed.  Thus a hostfile can list the same hosts multiple times, if more than one renderer is running on the same node.

	
	
	
	
	
	