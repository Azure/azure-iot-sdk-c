@REM Copyright (c) Microsoft. All rights reserved.
@REM Licensed under the MIT license. See LICENSE file in the project root for full license information.

setlocal
@setlocal EnableExtensions EnableDelayedExpansion
@echo off

rem -----------------------------------------------------------------------------
rem -- Setup Variables
rem -----------------------------------------------------------------------------
set CMAKE_DIR=iotsdk_win32
set BUILD_ARCH="Visual Studio 15 2017"
set build-platform=x86

set build-root=%~dp0..
rem // resolve to fully qualified path
for %%i in ("%build-root%") do set build-root=%%~fi

echo BUILD Root: %build-root%

:args-loop
if "%1" equ "" goto args-done
if "%1" equ "--platform" goto arg-build-platform
if "%1" equ "--cmake-root" goto arg-cmake-root
call :usage && exit /b 1

:arg-build-platform
shift
if "%1" equ "" call :usage && exit /b 1
set build-platform=%1
if %build-platform% == x64 (
    set BUILD_ARCH="Visual Studio 15 2017 Win64"
    set CMAKE_DIR=iotsdk_win64
) else if %build-platform% == arm (
    set BUILD_ARCH="Visual Studio 15 2017 ARM"
    set CMAKE_DIR=iotsdk_arm
)
goto args-continue

:args-continue
shift
goto args-loop

:args-done

REM -- Remove the cmake directory --
if EXIST %build-root%\cmake\%CMAKE_DIR% (
    rmdir /s/q %build-root%\cmake\%CMAKE_DIR%
    rem no error checking
)

echo CMAKE Output Path: %build-root%\cmake\%CMAKE_DIR%
mkdir %build-root%\cmake\%CMAKE_DIR%
pushd %build-root%\cmake\%CMAKE_DIR%

cmake -Drun_longhaul_tests:BOOL=OFF -Drun_e2e_tests:BOOL=ON -Drun_sfc_tests:BOOL=ON -Drun_unittests:BOOL=ON -Duse_c2d_amqp_methods:BOOL=ON -Duse_edge_modules=ON %build-root% -G %BUILD_ARCH%

msbuild /m azure_iot_sdks.sln
if !ERRORLEVEL! neq 0 exit /b !ERRORLEVEL!

if %build-platform% neq arm (
    ctest -C "debug" -V --schedule-random
    if not !ERRORLEVEL!==0 exit /b !ERRORLEVEL!
)

popd
goto :eof
 