@echo off

set BUILD_DIR=vs2022

if "%1"=="profile" (
	set BUILD_DIR=vs2022-profile
)

cmake --build %BUILD_DIR% --config Release --target HelixCompiler
