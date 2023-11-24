@echo off
setlocal EnableDelayedExpansion

set SOURCE_DIR=src
set BUILD_DIR=build
set OUT_DIR=bin

set INCLUDE_DIRS=/Isrc /I"%VULKAN_SDK%/Include"
set LIBS=user32.lib /LIBPATH:"%VULKAN_SDK%/Lib" vulkan-1.lib
set DEFINES=/D HKDEBUG /D HKDLL_OUT

set DLL_NAME=hikai

REM /Ehsc - Windows Structured Exception Handling
REM /W4   - Sets output warning level. Displays all warnings
REM /std: - Specifies c++ version
REM /Zi   - Debug Information Format
REM /LD   - Use Run-Time Library. Created DLL
REM /MD   - Use RTL. Uses multithread and DLL version of the run-time library. 
set COMPILER_FLAGS=/EHsc /W4 /std:c++17 /LD /MD

REM Set the path to the MSVC compiler
set COMPILER="N:\Apps\Programming\Visual Studio 2022 Community\VC\Tools\MSVC\14.37.32822\bin\Hostx64\x64\cl.exe"

REM Call vcvarsall.bat to set up environment variables
set VCVARSALL="N:\Apps\Programming\Visual Studio 2022 Community\VC\Auxiliary\Build\vcvarsall.bat"
@call %VCVARSALL% x64 >nul

if not exist %BUILD_DIR% mkdir %BUILD_DIR%
if not exist %OUT_DIR% mkdir %OUT_DIR%

REM Function to compile a directory of .cpp files
REM TODO: fix imgui and other files wrong  recompilation
:compile_dir
for /R %1 %%G in (*.cpp) do (
    set "source=%%G"
    set "object=%BUILD_DIR%/%%~nG.obj"

    if not exist "!object!" (
        set "recompile=true"
    ) else (
        for %%H in ("!source!") do set "source_time=%%~tH"
        for %%H in ("!object!") do set "object_time=%%~tH"

        if "!source_time!" GTR "!object_time!" (
            set "recompile=true"
        )
    )

    if defined recompile (
        echo Recompiling !source!
        @%COMPILER% %DEFINES% %COMPILER_FLAGS% %INCLUDE_DIRS% /c "!source!" ^
            /Fo:"!object!" 2>&1 | findstr /i "error warning"
        set "recompile="
    )
)
goto :linking

REM Compile each .cpp file in the source and imgui directories
call :compile_dir %SOURCE_DIR%
REM call :compile_dir %IMGUI_DIR%

:linking
REM Create an empty string to hold the list of .obj files
set OBJ_FILES=

REM Add each .obj file to the list
for /R %BUILD_DIR% %%G in (*.obj) do (
    set OBJ_FILES=!OBJ_FILES! %%G
)

REM Link the .obj files to create the executable
@link /DLL /DEBUG /OUT:%OUT_DIR%/%DLL_NAME%.dll /IMPLIB:bin/%DLL_NAME%.lib ^
                %OBJ_FILES% %LIBS% 2>&1 | findstr /i "error warning"

endlocal
