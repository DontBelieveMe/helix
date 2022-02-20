@echo off

pushd.
cd %~dp0\..
cmake --build vs2022 --target HelixCoreTests --config Release
vs2022\src\tests\Release\libhelix-tests.exe
popd
