@echo off
setlocal EnableDelayedExpansion

if exist "%~dp0bin" @RD /S /Q "%~dp0bin"

if exist "%~dp0engine\build" @RD /S /Q "%~dp0engine\build"
if exist "%~dp0engine\bin" @RD /S /Q "%~dp0engine\bin"

if exist "%~dp0game\build" @RD /S /Q "%~dp0game\build"
if exist "%~dp0game\bin" @RD /S /Q "%~dp0game\bin"

endlocal
