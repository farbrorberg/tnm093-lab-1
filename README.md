TNM093 Voreen module
===========

This is a simple [Voreen](http://voreen.org/) module for visualizing a [CT scan of a walnut](http://voreen.org/108-Data-Sets.html) made for the [TNM093 course (Practical Data Visualization and Virtual Reality) at Linköping University](http://www2.itn.liu.se/english/education/course/index.html?coursecode=TNM093).

The [workspace](workspaces/tnm093.vws) creates a QuadView with a [scatterplot view](src/tnm_scatterplot.cpp), a [parallell coordinates](src/tnm_parallelcoordinates.cpp) view, a slice view and a [3D model](src/tnm_raycaster.cpp) of the walnut.

The module also features a data reduction node and a couple of other neat things.
