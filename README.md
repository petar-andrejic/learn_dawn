In this repo, I'm following [Elie Michel's great tutorial](https://eliemichel.github.io/LearnWebGPU/) on webGPU in C++, but using Google's official C++ API from Dawn.

Some nice features this has over the C library are proper RAII automatic release etc for the reference counting, and slightly more comfortable async callback support. 


## Building
Requires pre-built Dawn as a dependency, I suggest following [Dawn's cmake](https://dawn.googlesource.com/dawn/+/HEAD/docs/quickstart-cmake.md) quickstart, and passing CMAKE_PREFIX_PATH=/path/to/your/dawn/

Make sure to run 
```git
git submodule update --init
```
to fetch the `glw3webgpu` dependency