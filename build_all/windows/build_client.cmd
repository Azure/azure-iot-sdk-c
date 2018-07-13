@REM Copyright (c) Microsoft. All rights reserved.
@REM Licensed under the MIT license. See LICENSE file in the project root for full license information.

@setlocal EnableExtensions EnableDelayedExpansion
@echo off

set current-path=%~dp0
rem // remove trailing slash
set current-path=%current-path:~0,-1%

set build-root=%current-path%\..\..
rem // resolve to fully qualified path
for %%i in ("%build-root%") do set build-root=%%~fi

rem -----------------------------------------------------------------------------
rem -- check prerequisites
rem -----------------------------------------------------------------------------

rem -----------------------------------------------------------------------------
rem -- parse script arguments
rem -----------------------------------------------------------------------------

rem // default build options
set build-clean=0
set build-config=
set build-platform=Win32
set CMAKE_build_python=OFF
set CMAKE_no_logging=OFF
set CMAKE_run_unittests=OFF
set prov_auth=OFF
set prov_use_tpm_simulator=OFF
set use_edge_modules=OFF

:args-loop
if "%1" equ "" goto args-done
if "%1" equ "--config" goto arg-build-config
if "%1" equ "--platform" goto arg-build-platform
if "%1" equ "--buildpython" goto arg-build-python
if "%1" equ "--no-logging" goto arg-no-logging
if "%1" equ "--run-unittests" goto arg-run-unittests
if "%1" equ "--provisioning" goto arg-provisioning
if "%1" equ "--use-tpm-simulator" goto arg-tpm-simulator
if "%1" equ "--use_edge_modules" goto arg-edge-modules
call :usage && exit /b 1

:arg-build-config
shift
if "%1" equ "" call :usage && exit /b 1
set build-config=%1
goto args-continue

:arg-build-platform
shift
if "%1" equ "" call :usage && exit /b 1
set build-platform=%1
goto args-continue

:arg-build-python
set CMAKE_build_python=2.7
if "%2"=="" goto args-continue
set PyVer=%2
if "%PyVer:~0,2%"=="--" goto args-continue
set CMAKE_build_python=%PyVer%
shift
goto args-continue

:arg-no-logging
set CMAKE_no_logging=ON 
goto args-continue

:arg-run-unittests
set CMAKE_run_unittests=ON
goto args-continue

:arg-provisioning
set prov_auth=ON
goto args-continue

:arg-tpm-simulator
set prov_use_tpm_simulator=ON
goto args-continue

:arg-edge-modules
set use_edge_modules=ON
goto args-continue

:args-continue
shift
goto args-loop

:args-done
set cmake-output=cmake_%build-platform%

rem -----------------------------------------------------------------------------
rem -- build with CMAKE
rem -----------------------------------------------------------------------------

echo CMAKE Output Path: %USERPROFILE%\%cmake-output%

rmdir /s/q %USERPROFILE%\%cmake-output%
rem no error checking

mkdir %USERPROFILE%\%cmake-output%
rem no error checking

pushd %USERPROFILE%\%cmake-output%

if %build-platform% == Win32 (
	echo ***Running CMAKE for Win32***
	cmake %build-root% -Drun_unittests:BOOL=%CMAKE_run_unittests% -Dbuild_python:STRING=%CMAKE_build_python% -Dno_logging:BOOL=%CMAKE_no_logging% -Duse_prov_client:BOOL=%prov_auth% -Duse_tpm_simulator:BOOL=%prov_use_tpm_simulator% -Duse_edge_modules=%use_edge_modules%
	if not !ERRORLEVEL!==0 exit /b !ERRORLEVEL!
) else (
	echo ***Running CMAKE for Win64***
	cmake %build-root% -G "Visual Studio 14 Win64" -Drun_unittests:BOOL=%CMAKE_run_unittests% -Dbuild_python:STRING=%CMAKE_build_python% -Dno_logging:BOOL=%CMAKE_no_logging% -Duse_prov_client:BOOL=%prov_auth% -Duse_tpm_simulator:BOOL=%prov_use_tpm_simulator%  -Duse_edge_modules=%use_edge_modules%
	if not !ERRORLEVEL!==0 exit /b !ERRORLEVEL!
)

if not defined build-config (
	echo ***Building both configurations***
	msbuild /m azure_iot_sdks.sln /p:Configuration=Release
	if not !ERRORLEVEL!==0 exit /b !ERRORLEVEL!
	msbuild /m azure_iot_sdks.sln /p:Configuration=Debug
	if not !ERRORLEVEL!==0 exit /b !ERRORLEVEL!
) else (
	echo ***Building %build-config% only***
	msbuild /m azure_iot_sdks.sln /p:Configuration=%build-config%
	if not !ERRORLEVEL!==0 exit /b !ERRORLEVEL!
)

popd
goto :eof


:usage
echo build_client.cmd [options]
echo options:
echo  --config ^<value^>         [Debug] build configuration (e.g. Debug, Release)
echo  --platform ^<value^>       [Win32] build platform (e.g. Win32, x64, ...)
echo  --buildpython ^<value^>    [2.7]   build python extension (e.g. 2.7, 3.4, ...)
echo  --no-logging               Disable logging
echo  --run-unittests            Run unittests
echo  --provisioning             Use Provisiong service
echo  --use-tpm-simulator        Build TPM simulator
goto :eof
