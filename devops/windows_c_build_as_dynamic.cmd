@REM Copyright (c) Microsoft. All rights reserved.
@REM Licensed under the MIT license. See LICENSE file in the project root for full license information.

@setlocal EnableExtensions EnableDelayedExpansion
@echo off

ver
msbuild -version

set current-path=%~dp0
rem // remove trailing slash
set current-path=%current-path:~0,-1%

set build-root=%current-path%\..
rem // resolve to fully qualified path
for %%i in ("%build-root%") do set build-root=%%~fi

echo Build Root: %build-root%
set cmake-root=%build-root%

set CMAKE_DIR=dyn

@REM :args-loop
REM if "%1" equ "--platform" goto arg-build-platform
REM call :usage && exit /b 1

if EXIST %cmake-root%\cmake\%CMAKE_DIR% (
    rmdir /s/q %cmake-root%\cmake\%CMAKE_DIR%
    rem no error checking
)

echo CMAKE Output Path: %cmake-root%\cmake\%CMAKE_DIR%
mkdir %cmake-root%\cmake\%CMAKE_DIR%
rem no error checking
pushd %cmake-root%\cmake\%CMAKE_DIR%

echo ***Running CMAKE for building dynamic***
cmake %build-root% -Dbuild_as_dynamic:BOOL=ON -Duse_edge_modules=ON
if not !ERRORLEVEL!==0 exit /b !ERRORLEVEL!
popd

msbuild /m %cmake-root%\cmake\%CMAKE_DIR%\azure_iot_sdks.sln
if !ERRORLEVEL! neq 0 exit /b !ERRORLEVEL!
