# Tracer

The files in this directory implement Runge-Kutta particle advection and various tools that aid in visualizing those path lines.  

  * **RungeKutta.cpp**, **RungeKutta.h** implements distributed-memory Runge-Kutte4 particle advection.rParticles are traced in whichever vector-field partition that contains the current head of the particle trace, and when a boundary is encountered, a partial trace is retained in the current process and a message is sent to continue the trace on the neighbor across the boundary (if there is one)   The inputs are a particle set, a vector field, and various parameters; the output is a set of particle traces distributed similarly to the underlying vector field.  Note that each trace in particle trace data set may consist of several segments if the particle re-enters a partition of the vector field where its already been.
  * **TraceToPathLines.cpp**, **TracetoPathLines.h** implement converting structured particle traces to simple renderable path lines.  It allows two parameters: a time *t* and a delta-time *dt*; if given, only the portion of the streamline with integration time between 	(*t* - *dt*) and *t.
  * **Interpolator.cpp**, **Interpolator.h** interpolate a scalar volume dataset onto a Geometry dataset - eg. either particles or pathlines.

The example application *sampletrace* is included in **sampletrace.cpp**.  This application implements a multi-step workflow that:

1. Reads an initial set of datasets by processing the input *data.state* file
2. Uses a sampler (see *../sample*) to sample a given volume.   This operation is specified in the *sample.state* input file.   The dataset to be sampled is specified therein.  The result of this step is a Particles dataset named 'samples'.
2. Optionally interpolates an arbitrary scalar volume dataset onto the samples particles.   This dataset is specied by the *-sdata name* command-line parameter.
3. Runs the **RungeKutta** operator to advect particle traces.   It expects to find a vector dataset named *vectors*.  It then produces particle traces from the samples produced earlier.  The results are stored in the RungeKutta object.
4. Runs **TraceToPathLines** to convert some or all of the particle traces to renderable pathlines
5.  Optionally interpolates an arbitrary scalar volume dataset onto the resulting pathlines.   This dataset is specied by the *-pdata name* command-line parameter.
6. Renders images based on the *render.state* file given on the command line.

Note that steps 5, 6, and 7 may run in a loop (controlled by the command-line *-nf* and *-dt* arguments) that enable rendering movies where the particle traces are animated in integration time.

For a complete set of command-line args, please run **sampletrace --**.  See the *sampletrace* directory in the examples.
