This is an example of sampling a volumetric scalar field using the IsoSampler vis.   It makes use of the sampletrace application, which contains two raycasting passes: first one to sample and finally one to render, separated by a hard-coded application of the RungeKutta operator followed by the TraceToPathLines operator to trace pathlines from the particles through a vector volume field, then convert the path lines to renderable form.

The data is available in Box at: https://utexas.box.com/s/1vz5i89x2dqin92y9fxu5r0nbk0yzo0q

First, run ./setup_data   This extracts the necessary scalar and vector volumes from the original VTI.

Then run ./run to generate 8 images of streamlines traced through the vector field from samples of the scalar field.
