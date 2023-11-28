@echo off
setlocal EnableDelayedExpansion

echo ===========================
echo     Hikai + Game Build
echo ===========================

echo Building engine...
echo ---------------------------

pushd engine
call build.bat
popd
echo ---------------------------

echo Building game...
echo ---------------------------

pushd game
call build.bat
popd
echo ---------------------------


if not exist bin mkdir bin

echo Copying hikai.dll...
robocopy ".\engine\bin" ".\bin" hikai.dll /mt 2>&1 | findstr /i "ERROR"

echo Copying blight.exe...
robocopy ".\game\bin" ".\bin" blight.exe /mt 2>&1 | findstr /i "ERROR"

echo ===========================
echo Build successful

endlocal
