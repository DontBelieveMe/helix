@echo off

pushd.
cd %~dp0\..
cmake --build vs2022 --target HelixCoreTests HelixCompiler --config Release
call tools\Testify
vs2022\src\tests\Release\libhelix-tests.exe
popd
