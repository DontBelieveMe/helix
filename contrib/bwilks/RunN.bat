@echo off
set LAST=0
setlocal enabledelayedexpansion

set ok=true

FOR /L %%G IN (1,1,40) DO (
	echo "Compiling 'testsuite\f2\f2010-struct-initializers.c' -> 'temp/%%G.txt'"
	call .\tools\hxc testsuite\f2\f2010-struct-initializers.c > temp\%%G.txt 2>&1 

	if %%G NEQ 1 (
		fc temp\%%G.txt temp\!LAST!.txt >nul

		if ERRORLEVEL 1 (
			echo "temp\%%G.txt temp\!LAST!.txt are different"
			set ok=false
		)
	)

	set LAST=%%G
)

echo ok? !ok!