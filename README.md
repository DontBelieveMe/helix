# Helix Compiler
## Overview
> Compiler Optimization Techniques

An optimising compiler for a subset of the C programming language. The project is specifically concerned with program optimisation and machine code generation. Clang is utilised as the C frontend, and the GCC ARM (Windows Cross) toolchain is utilised for assembling the output assembly, linking and as a runtime standard library (in whatever capacity it can be used).

Developed as the final year project for a BSc Computer Science degree at Sheffield Hallam University.

## Technical
### Requirements

#### External

 - C++17 compatible compiler
    - Only currently tested under Visual Studio 2022 (previously VS2019, so that should still work)
 - CMake (only tested at latest version, `3.22.1`)
 - Precompiled LibTooling (Clang) binaries
    - Run `tools/BuildLLVM_Release.bat` (or `tools/BuildLLVM_Debug.bat` depending on your configuration of choice)
      and this should compile all the clang/llvm binaries for you and put them into the right places
      in the `lib/` folder (also will sort out copying any autogenerated headers that are needed).


### Building

Compiling the full compiler is currently only possible under Windows, but it is possible
to compile `HelixCore` and `HelixCoreTests` under linux (supported for generating code coverage
information of the core libraries).

#### Visual Studio 2022

In a terminal, from the project root.

- Ensure all submodules in `external/` are cloned (LLVM is pretty big so using `--depth=1` to only
  fetch the latest commit is recommended to keep time/disk space down).

- Build LLVM using by running `tools/BuildLLVM_Release.bat` (or `tools/BuildLLVM_Debug.bat` if wanting
  to compile in Debug mode)

- To generate the project files into a `vs2022/` folder `cmake -S. -Bvs2022 -G"Visual Studio 2022"`
  - If using the Release mode LLVM binaries, make sure to add `-DCMAKE_BUILD_TYPE=Release` to the above `cmake`
    command.

- To build either
  - Open Visual Studio, set `HelixCompiler` as the startup project and build as normal, or...
  - `cmake --build vs2022 --target HelixCompiler` to build via CMake from the command line
  - Make sure to build in the same configuration as LLVM was built in, if building from the command line
    add `--configuration Release`.

#### Linux

Pretty much identical to windows, but you don't need to compile LLVM/Clang.

### Internal Details

#### Compiler Internals 

##### Core

- `HelixCore` - Core compiler libraries, middle/backend of the compiler.
- `HelixCoreTests` - Unit tests for `HelixCore`.
- `HelixClangFrontend` - C compiler frontend, uses clang to parse C and generate IR, passed to `HelixCore`.
  - Also currently in charge of parsing and managing command line parameters (defined in `options.def`), this
    is because the LLVM `cl` library is currently used to do this. Ultimately this should go in the compiler
    driver instead, and `HelixClangFrontend` should just be concerned with converting C -> IR.
- `HelixCompiler` - The compiler "driver" system, basically has the job of combining `Helix*Frontend` to generate
   IR, then `HelixCore` to convert that to assembly, and finally using external system tools (like GNU Tools/GCC or Clang)
   to assemble and link the produced assembly into executable machine code.

##### Tools
 - `MachineDescription` - Tool that parses the S-Exp like machine description files (e.g. `src/arm.md` and generating
   C++ that matches those patterns against the IR and produces assembly code)
 - `Testify` - Tool that is the test runner for all the functional/"integration tests" (doesn't handle unit tests yet,
   but manages the testing of everything else). E.g. is used to run the internal IR tests (defined in `testsuite/`)
   and any configured external testsuite (e.g. `c-testsuite` conformance tests).

#### Submodules

- `llvm-project` submodule, required for the clang C frontend.
  - Binaries not produced as part of the build process and the build system expects them in a certain layout
    in the `lib/` folder. `tools/BuildLLVM_[Debug|Release].bat` will sort all this out automatically.
- `spdlog` submodule, used for logging (also use is made of the bundled fomatlib).
