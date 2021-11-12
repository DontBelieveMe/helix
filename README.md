# Helix Compiler

Sheffield Hallam BSc Computer Science final year project.

Optimising compiler backend, built on clang.

## Requirements

### External

 - C++17 compatible compiler
    - Only tested under Visual Studio 2019
 - CMake
 - Precompiled LibTooling (Clang) binaries
    - Should be placed in the `lib/` directory in the following format:
       - Static library binaries should go in `lib/bin`
       - Headers produced by the Clang/LLVM build should go in `lib/include`

### Internal

- `llvm-project` provided as a submodule, required for headers.
  - Binaries not produced as part of the build process and need to be compiled & copied to the `lib/` folder separately. I've already done this once and can upload `lib/` to google drive or somewhere where it makes sense to store lots of binary data that's not GitHub.

## Building

### Visual Studio 2019

In a terminal, from the project root.

- To generate the project files into a `vs2019/` folder `cmake -S. -Bvs2019`
- To build either
  - Open Visual Studio, set `helix` as the startup project and build as normal, or...
  - `cmake --build vs2019 --target helix` to build via CMake from the command line