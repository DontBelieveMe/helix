@echo off
call %~dp0\hxc.bat %~dp0\..\extras\helix-benchmark\src\%1\%1.c -D benchmark_main=main %2 %3 %4
wsl ./tools/run-arm.sh ./a.out

