@echo off
pushd .
cd %~dp0\..\external\llvm-project

echo Configuring CMake...
cmake -Sllvm -Bbuild-release -A x64 -Thost=x64 ^
    -DLLVM_ENABLE_PROJECTS=clang ^
    -DLLVM_TARGETS_TO_BUILD=X86 ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DLLVM_ENABLE_ASSERTIONS=ON ^
    -DLLVM_ENABLE_ZLIB=OFF

echo Building Binaries...
cmake --build build-release ^
      --target clangAST ^
      --config Release ^
clangASTMatchers ^
clangAnalysis ^
clangBasic ^
clangDriver ^
clangEdit ^
clangFrontend ^
clangLex ^
clangParse ^
clangRewrite ^
clangSema ^
clangSerialization ^
clangTooling ^
clangToolingCore ^
clangToolingSyntax

xcopy build-debug\Release\lib\*.lib ..\..\lib\bin\ /S /Y

xcopy build-debug\include\*.h ..\..\lib\include\ /S /y
xcopy build-debug\include\*.def ..\..\lib\include\ /S /y
xcopy build-debug\include\*.inc ..\..\lib\include\ /S /y

xcopy build-debug\tools\clang\include\*.h ..\..\lib\include\ /S /y
xcopy build-debug\tools\clang\include\*.def ..\..\lib\include\ /S /y
xcopy build-debug\tools\clang\include\*.inc ..\..\lib\include\ /S /y

popd
