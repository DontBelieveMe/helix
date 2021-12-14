@echo off
%~dp0\..\vs2019\Release\helix.exe %* --

if exist t.S (
	%~dp0\..\contrib\bwilks\gcc-arm\bin\arm-none-linux-gnueabihf-gcc.exe -static t.S
)
