@echo off
setlocal EnableDelayedExpansion

if exist "%~dp0bin" @RD /S /Q "%~dp0bin"

if exist "%~dp0engine\build" @RD /S /Q "%~dp0engine\build"
if exist "%~dp0engine\bin"   @RD /S /Q "%~dp0engine\bin"

if exist "%~dp0editor\build" @RD /S /Q "%~dp0editor\build"
if exist "%~dp0editor\bin"   @RD /S /Q "%~dp0editor\bin"

if exist "%~dp0sandbox\build" @RD /S /Q "%~dp0sandbox\build"
if exist "%~dp0sandbox\bin"   @RD /S /Q "%~dp0sandbox\bin"

endlocal
