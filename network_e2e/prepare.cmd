@REM Copyright (c) Microsoft. All rights reserved.
@REM Licensed under the MIT license. See LICENSE file in the project root for full license information.

@setlocal EnableExtensions EnableDelayedExpansion

set script_dir=%~dp0
rem // remove trailing slash
for %%i in ("%script_dir%") do set script_dir=%%~fi

set build_root=%script_dir%\..
rem // resolve to fully qualified path
for %%i in ("%build_root%") do set build_root=%%~fi

set build_folder=%build_root%\cmake\iotsdk_win32


rd /q /s %build_folder%
mkdir %build_folder%
if !ERRORLEVEL! neq 0 exit /b !ERRORLEVEL!

pushd %build_folder%
if !ERRORLEVEL! neq 0 exit /b !ERRORLEVEL!

cmake ^
  -Drun_e2e_tests:BOOL=ON ^
  -Duse_wsio:BOOL=ON ^
  -Drun_unittests:BOOL=ON ^
  -Dbuild_network_e2e:BOOL=ON ^
  %build_root%
if !ERRORLEVEL! neq 0 exit /b !ERRORLEVEL!

cd %build_folder%\network_e2e\tests
msbuild /m  iothubclient_badnetwork_e2e\iothubclient_badnetwork_e2e_exe.vcxproj
rem if !ERRORLEVEL! neq 0 exit /b !ERRORLEVEL!

