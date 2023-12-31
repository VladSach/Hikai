@echo off
setlocal EnableDelayedExpansion

echo ============================
echo    Hikai + Sandbox Build
echo ============================

echo Building engine...
echo ----------------------------

pushd engine
call build.bat
popd
echo ----------------------------

echo Building sandbox...
echo ----------------------------

pushd sandbox
call build.bat
popd
echo ----------------------------


if not exist bin mkdir bin

echo Copying hikai.dll...
robocopy ".\engine\bin" ".\bin" hikai.dll /mt 2>&1 | findstr /i "ERROR"

echo Copying dxcompiler.dll...
robocopy ".\engine\lib" ".\bin" dxcompiler.dll /mt 2>&1 | findstr /i "ERROR"

echo Copying sandbox.exe...
robocopy ".\sandbox\bin" ".\bin" sandbox.exe /mt 2>&1 | findstr /i "ERROR"

echo ============================
echo Build successful

endlocal
