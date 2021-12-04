@echo off

set COMMON_PREFIX=%~dp0Testify\bin
set COMMON_POSTFIX=netcoreapp3.1\Testify.exe

set DEBUG_TESTIFY=%COMMON_PREFIX%\Debug\%COMMON_POSTFIX%
set RELEASE_TESTIFY=%COMMON_PREFIX%\Release\%COMMON_POSTFIX%

if exist %RELEASE_TESTIFY% (
    set TESTIFY_EXE=%RELEASE_TESTIFY%
) else (
    if exist %DEBUG_TESTIFY% (
        set TESTIFY_EXE=%DEBUG_TESTIFY%
    ) else (
        echo Cannot find Testify.exe
        exit
    )
)

echo Found Testify at '%TESTIFY_EXE%'
echo.

%TESTIFY_EXE% %*
