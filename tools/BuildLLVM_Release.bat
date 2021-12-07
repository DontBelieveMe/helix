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
      --parallel 7 ^
      --config Release ^
      --target clangAST ^
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

xcopy build-release\Release\lib\*.lib ..\..\lib\bin\ /S /Y

xcopy build-release\include\*.h ..\..\lib\include\ /S /y
xcopy build-release\include\*.def ..\..\lib\include\ /S /y
xcopy build-release\include\*.inc ..\..\lib\include\ /S /y

xcopy build-release\tools\clang\include\*.h ..\..\lib\include\ /S /y
xcopy build-release\tools\clang\include\*.def ..\..\lib\include\ /S /y
xcopy build-release\tools\clang\include\*.inc ..\..\lib\include\ /S /y

popd
