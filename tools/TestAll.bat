@echo off

pushd.
cd %~dp0\..
cmake --build vs2019 --target unit-tests helix
call tools\Testify
vs2019\Debug\unit-tests.exe
popd