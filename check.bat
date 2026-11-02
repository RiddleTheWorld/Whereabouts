@echo off
rem Builds and runs the development checks. Exit code is non-zero on failure.
rem
rem   check.bat            end-to-end, window controls, and a real capture
rem   check.bat e2e        capture -> save -> reload -> lay out
rem   check.bat windows    every title-bar control and the shortcuts
rem   check.bat capture    prints what a real capture of this machine recorded
rem   check.bat regress    regression checks for previously fixed flaws
rem   check.bat filter     kind-filter chips
rem   check.bat keys       keyboard navigation
rem   check.bat sort       list sorting
rem   check.bat geom       window geometry persistence
rem   check.bat audit      bug-fix regressions
rem   check.bat live       live folder updates
rem   check.bat move       moving and renaming while open
rem   check.bat desk       desktop icon-position polling
rem   check.bat debug      bugs found in the debug pass
rem   check.bat scroll     scrolling, list hit-testing, and hover
setlocal

set "QT_DIR=C:/Qt/6.12.0/mingw_64"
set "MINGW_DIR=C:\Qt\Tools\mingw1310_64"
set "CMAKE_DIR=C:\Qt\Tools\CMake_64"
set "NINJA_DIR=C:\Qt\Tools\Ninja"
set "BUILD=build"
set "ONLY=%~1"

set "PATH=%CMAKE_DIR%\bin;%NINJA_DIR%;%MINGW_DIR%\bin;%PATH%"
rem The tool binaries live in build\tools, but the Qt runtime and its platform
rem plugin are staged one level up beside the app. PATH alone is not enough:
rem Qt also looks for platforms\qwindows.dll relative to the executable, so the
rem plugin path has to be given explicitly or every tool aborts at startup with
rem "no Qt platform plugin could be initialized".
set "PATH=%CD%\%BUILD%;%QT_DIR%\bin;%PATH%"
set "QT_QPA_PLATFORM_PLUGIN_PATH=%CD%\%BUILD%\platforms"

if not exist %BUILD%\CMakeCache.txt (
  echo Run build.bat first.
  exit /b 1
)

set "FAILED=0"
if "%ONLY%"=="" call :run e2e
if "%ONLY%"=="" call :run windowprobe
if "%ONLY%"=="" call :run capturecheck
if "%ONLY%"=="" call :run regress
if "%ONLY%"=="" call :run filtercheck
if "%ONLY%"=="" call :run keycheck
if "%ONLY%"=="" call :run sortcheck
if "%ONLY%"=="" call :run geomcheck
if "%ONLY%"=="" call :run auditcheck
if "%ONLY%"=="" call :run livecheck
if "%ONLY%"=="" call :run movecheck
if "%ONLY%"=="" call :run deskcheck
if "%ONLY%"=="" call :run debugcheck
if "%ONLY%"=="" call :run scrollcheck
if /i "%ONLY%"=="e2e"     call :run e2e
if /i "%ONLY%"=="windows" call :run windowprobe
if /i "%ONLY%"=="capture" call :run capturecheck
if /i "%ONLY%"=="regress" call :run regress
if /i "%ONLY%"=="filter" call :run filtercheck
if /i "%ONLY%"=="keys" call :run keycheck
if /i "%ONLY%"=="sort" call :run sortcheck
if /i "%ONLY%"=="geom" call :run geomcheck
if /i "%ONLY%"=="audit" call :run auditcheck
if /i "%ONLY%"=="live"  call :run livecheck
if /i "%ONLY%"=="move"  call :run movecheck
if /i "%ONLY%"=="desk"  call :run deskcheck
if /i "%ONLY%"=="debug" call :run debugcheck
if /i "%ONLY%"=="scroll" call :run scrollcheck

if %FAILED% neq 0 (
  echo.
  echo SOME CHECKS FAILED
  exit /b 1
)
echo.
echo ALL CHECKS PASSED
exit /b 0

:run
set "NAME=%~1"
"%CMAKE_DIR%\bin\cmake.exe" --build %BUILD% --target cairn_%NAME%
if not "%errorlevel%"=="0" (
  echo   build failed for cairn_%NAME%
  set "FAILED=1"
  exit /b 0
)
pushd %BUILD%\tools
cairn_%NAME%.exe
set "CODE=%errorlevel%"
popd
rem Compare against 0 exactly. Windows crash codes such as 0xC0000602 arrive as
rem NEGATIVE numbers, so "if errorlevel 1" -- which means ">= 1" -- treats a
rem hard crash as success. This is how a broken tool once reported PASSED.
if not "%CODE%"=="0" (
  echo   cairn_%NAME%.exe failed with exit code %CODE%
  set "FAILED=1"
)
exit /b 0
