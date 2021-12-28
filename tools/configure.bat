@echo off

cmake -S. -G"Visual Studio 17" -DCMAKE_BUILD_TYPE=Release -DCONFIG_GNU_TOOLS_ROOT=C:/helix/contrib/bwilks/gcc-arm/bin %*
