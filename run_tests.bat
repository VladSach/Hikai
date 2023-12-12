@echo off
setlocal

echo ===========================
echo        Hikai Test
echo ===========================

robocopy ".\engine\bin" ".\tests" hikai.dll /mt 2>&1 | findstr /i "ERROR"

REM Sets environment variable "VCROOT" to  Drive:\Path\To\VS\VC.
pushd %~dp0.\engine\tools
call locateMSVC.bat
popd
if errorlevel 1 exit /B %ERRORLEVEL%

REM Call vcvarsall.bat to set up environment variables
call "%VCROOT%\Auxiliary\Build\vcvarsall.bat" x64 >nul

echo "Compiling tests..."
pushd tests
cl entry.cpp UnitTest.cpp Tests.cpp ^
   /EHsc /W4 /std:c++17 /Fe:tests ^
   /I"../engine/src" ^
   /link /LIBPATH:"../engine/bin" hikai.lib ^
   2>&1 | findstr /i "error warning"

echo "Running tests..."
.\tests.exe

echo:
type results.txt
echo:

echo "Cleanup..."
if exist *.obj DEL /F *.obj
if exist tests.exe DEL /F tests.exe
if exist hikai.dll DEL /F hikai.dll
popd REM tests

endlocal
