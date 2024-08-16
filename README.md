In this repo, I'm following [Elie Michel's great tutorial](https://eliemichel.github.io/LearnWebGPU/basic-3d-rendering/input-geometry/playing-with-buffers.html) on webGPU in C++, but using Google's official C++ API from Dawn.

Some nice features this has over the C library are proper RAII automatic release etc for the reference counting, and slightly more comfortable async callback support. 
