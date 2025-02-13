@echo off
setlocal EnableDelayedExpansion

echo ============================
echo         Hikai Build
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

if not exist bin mkdir bin

echo Copying hikai.dll...
robocopy ".\engine\bin" ".\bin" hikai.dll /R:0 /W:0 /mt 2>&1 | findstr /i "ERROR"
robocopy ".\engine\bin" ".\bin" hikai.pdb /mt 2>&1 | findstr /i "ERROR"

echo Copying dxcompiler.dll...
robocopy ".\engine\lib" ".\bin" dxcompiler.dll /mt 2>&1 | findstr /i "ERROR"

echo Copying assimp-vc143-mt.dll...
robocopy ".\engine\lib" ".\bin" assimp-vc143-mt.dll /mt 2>&1 | findstr /i "ERROR"

echo Copying editor.exe...
robocopy ".\editor\bin" ".\bin" editor.exe /mt 2>&1 | findstr /i "ERROR"
robocopy ".\editor\bin" ".\bin" editor.pdb /mt 2>&1 | findstr /i "ERROR"

echo ============================
echo Build successful

endlocal
