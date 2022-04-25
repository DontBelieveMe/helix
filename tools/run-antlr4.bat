@echo off

call %~dp0\antlr4.bat -Dlanguage=Cpp %~dp0\..\src\irc\IR.g4 -o %~dp0\..\src\irc -package "Helix::Frontend::IR" -visitor -no-listener
