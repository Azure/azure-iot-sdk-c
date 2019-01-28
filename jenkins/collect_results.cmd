@REM Copyright (c) Microsoft. All rights reserved.
@REM Licensed under the MIT license. See LICENSE file in the project root for full license information.

setlocal

set working-dir=%~dp0..
rem // resolve to fully qualified path

for %%i in ("%working-dir%") do set working-dir=%%~fi

echo %working-dir%

REM -- C --

pushd "%working-dir%"

Powershell.exe -executionpolicy remotesigned -File  %working-dir%\jenkins\ctest_to_junit.ps1 %1
if not %ERRORLEVEL%==0 exit /b %ERRORLEVEL%

popd