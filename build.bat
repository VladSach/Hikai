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

echo Building editor...
echo ----------------------------

pushd editor
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

echo Copying editor.exe...
robocopy ".\editor\bin" ".\bin" hikai.exe /mt 2>&1 | findstr /i "ERROR"

echo Copying sandbox.exe...
robocopy ".\sandbox\bin" ".\bin" sandbox.exe /mt 2>&1 | findstr /i "ERROR"

echo Copying assets...
robocopy ".\engine\assets" ".\bin\assets" /s /mt 2>&1 | findstr /i "ERROR"

echo ============================
echo Build successful

endlocal
