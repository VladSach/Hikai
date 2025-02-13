@echo off
setlocal EnableDelayedExpansion

set SOURCE_DIR=src
set BUILD_DIR=build
set OUT_DIR=bin

set INCLUDE_DIRS=/Isrc /I"../engine/src" /I"../engine/src/vendor"
set LIBS=/LIBPATH:"../engine/bin" hikai.lib
set DEFINES=/D HKDEBUG

set EXE_NAME=editor.exe

REM /Ehsc - Windows Structured Exception Handling
REM /W4   - Sets output warning level. Displays all warnings
REM /std: - Specifies c++ version
REM /Z7   - Debug Information Format
set COMPILER_FLAGS=/EHsc /W4 /std:c++17 /Z7

REM Sets environment variable "VCROOT" to  Drive:\Path\To\VS\VC.
pushd %~dp0..\engine\tools
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
for /R %SOURCE_DIR% %%G in (*.cpp) do (
    if "%%~xG"==".cpp" (
        set "source=%%G"
        set "object=%BUILD_DIR%/%%~nG.obj"

        if not exist "!object!" (
            echo Compiling !source!
            set "recompile=true"
        ) else (
            for %%H in ("!source!") do set "sdate=%%~tH"
            for %%H in ("!object!") do set "odate=%%~tH"

            REM Extract just the filename from the full path
            for %%N in ("!source!") do set "sfilename=%%~nxN"

            REM Change directory to where the source file is located
            pushd "%%~dpG"
            for /f "delims=" %%i in ('forfiles /m "!sfilename!" /c "cmd /c echo @ftime"') do set stime=%%i
            popd
            pushd "%BUILD_DIR%"
            for /f "delims=" %%i in ('forfiles /m "%%~nG.obj" /c "cmd /c echo @ftime"') do set otime=%%i
            popd

            REM FIX: nonportable solution
            for /F "tokens=1-3 delims=/ " %%T in ("!sdate!") do (
                set "sday=%%T"
                set "smonth=%%U"
                set "syear=%%V"
            )
            for /F "tokens=1-3 delims=/ " %%T in ("!odate!") do (
                set "oday=%%T"
                set "omonth=%%U"
                set "oyear=%%V"
            )

            for /F "tokens=1-3 delims=:" %%T in ("!stime!") do (
                set "shour=%%T"
                set "sminute=%%U"
                set "ssec=%%V"
            )
            for /F "tokens=1-3 delims=:" %%T in ("!otime!") do (
                set "ohour=%%T"
                set "ominute=%%U"
                set "osec=%%V"
            )

            pushd %~dp0..\engine\tools
            call DateToSecs.bat DateToSecs ^
            !syear! !smonth! !sday! !shour! !sminute! !ssec! ssecs
            call DateToSecs.bat DateToSecs ^
            !oyear! !omonth! !oday! !ohour! !ominute! !osec! osecs
            popd

            if "!ssecs!" GTR "!osecs!" (
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
@link /DEBUG:FULL /OUT:%OUT_DIR%/%EXE_NAME% %OBJ_FILES% %LIBS% 2>&1 | findstr /i "error warning"

endlocal
