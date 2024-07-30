@echo off
setlocal EnableDelayedExpansion

set SOURCE_DIR=src
set BUILD_DIR=build
set OUT_DIR=bin
set LIB_DIR=lib

set INCLUDE_DIRS=/Isrc /Isrc/vendor/
set LIBS=user32.lib /LIBPATH:"%LIB_DIR%" vulkan-1.lib dxcompiler.lib
set DEFINES=/D HKDEBUG /D HKDLL_OUT

set DLL_NAME=hikai

REM /Ehsc - Windows Structured Exception Handling
REM /W4   - Sets output warning level. Displays all warnings
REM /std: - Specifies c++ version
REM /Z7   - Debug Information Format
REM /LD   - Use Run-Time Library. Created DLL
REM /MD   - Use RTL. Uses multithread and DLL version of the run-time library.
REM /Zc:preprocessor - MSVC new cross-platform compatible preprocessor
set COMPILER_FLAGS=/EHsc /W4 /std:c++17 /Z7 /LDd /MD /Zc:preprocessor


REM Sets environment variable "VCROOT" to  Drive:\Path\To\VS\VC.
pushd tools
call locateMSVC.bat
popd
if errorlevel 1 exit /B %ERRORLEVEL%

REM Call vcvarsall.bat to set up environment variables
@call "%VCROOT%\Auxiliary\Build\vcvarsall.bat" x64 >nul

REM Set the path to the MSVC compiler
set COMPILER="cl"

if not exist %BUILD_DIR% mkdir %BUILD_DIR%
if not exist %OUT_DIR% mkdir %OUT_DIR%

REM Function to compile a directory of .cpp files

REM I hate cppm
REM for /R %SOURCE_DIR% %%G in (*.cpp) do (
for /R %SOURCE_DIR% %%G in (*) do (
    if "%%~xG"==".cpp" (
        set "source=%%G"
        set "object=%BUILD_DIR%/%%~nG.obj"

        if not exist "!object!" (
            echo Compiling !source!
            set "recompile=true"
        ) else (
            for %%H in ("!source!") do set "source_time=%%~tH"
            for %%H in ("!object!") do set "object_time=%%~tH"

            REM for /f "delims=" %%i in ('dir /b /twc "!source!"') do set "source_time=%%~ti"
            REM for /f "delims=" %%i in ('dir /b /twc "!object!"') do set "object_time=%%~ti"

            if "!source_time!" GTR "!object_time!" (
                echo Recompiling !source!
                set "recompile=true"
            ) else (
                set "recompile="
            )
        )

        if defined recompile (
            @%COMPILER% %DEFINES% %COMPILER_FLAGS% %INCLUDE_DIRS% /c "!source!" ^
                /Fo:"!object!" 2>&1 | findstr /i "error warning"
            set "recompile="
        )
    )
)

REM Create an empty string to hold the list of .obj files
set OBJ_FILES=

REM Add each .obj file to the list
for /R %BUILD_DIR% %%G in (*.obj) do (
    set OBJ_FILES=!OBJ_FILES! %%G
)

REM Link the .obj files to create the executable
@link /DLL /DEBUG:FULL ^
           /OUT:%OUT_DIR%/%DLL_NAME%.dll /IMPLIB:bin/%DLL_NAME%.lib ^
           %OBJ_FILES% %LIBS% 2>&1 | findstr /i "error warning"

endlocal
