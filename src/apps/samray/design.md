# Notes on adapting galaxy to ray-based sampling

The overall task is to adapt galaxy so that we can prototype different sampling approaches at scale.

Our current prototype samples at an isosurface boundary. This is an operation defined by Galaxy's current design, which uses the classes `Datasets`, `Visualization`, `VolumeVis`, `RenderingSet`, `Camera`, `Rendering` in conjunction with the `Sampler` class to generate sampled data.

In general, there are two approaches we can pursue. It's likely that we should pursue a combination of both, in the interests of ease of prototyping, execution efficiency and performance integrity of the existing Galaxy code.

- **Adaptation of the current framework**. This means that we adapt our current classes and operations to support ray-based sampling
- **Support for a sample-based workflow**. This means that we develop a workflow based on sampling operations, in which samples are both the input and the output. For example, a sampling operation is performed, and the output of that operation is used to seed ray generation for a next step. This is the approach spelled out in the paper, and it also fits well with the design of galaxy `Datasets`

This document is the start of the design discussion to determine our path forward.

## Overview

Per the project proposal, we'd like to be able to use a ray-based approach to intelligent sampling. This means we need to be able to control the following:

- **Initial ray creation**. Determining how the initial set of rays is created.
	- Looking at the code, it appears that if we make the `Camera::spawn_rays_task` a member variable instead of a nested class, we will be able to create different methods of spawning rays. In particular, we would like the following:
		- orthographic projection
		- Control over the gemoetry boundary of the projection (circular, arbitrary, etc.)
- **Ray event processing**. Determining when a ray hits something of interest, and what is done in response.
- **Ray propogation**. Determining when a ray's path is altered (to follow a gradient, for example), stopped, or new rays are spawned at a specific point.

