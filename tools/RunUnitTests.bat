@echo off

pushd.
cd %~dp0\..
cmake --build vs2019 --target LibHelixTests Helix --config Release
vs2019\Release\libhelix-tests.exe
popd