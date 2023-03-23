@REM Copyright (c) Microsoft. All rights reserved.
@REM Licensed under the MIT license. See LICENSE file in the project root for full license information.

echo ***setting VC paths***
IF EXIST "%ProgramFiles(x86)%\Microsoft Visual Studio\2017\Enterprise\Common7\Tools\VsMSBuildCmd.bat" ( 
    call "%ProgramFiles(x86)%\Microsoft Visual Studio\2017\Enterprise\Common7\Tools\VsMSBuildCmd.bat"
    set VSVERSION="Visual Studio 15 2017"
) ELSE IF EXIST "%ProgramFiles%\Microsoft Visual Studio\2022\Enterprise\Common7\Tools\VsMSBuildCmd.bat" (
    call "%ProgramFiles%\Microsoft Visual Studio\2022\Enterprise\Common7\Tools\VsMSBuildCmd.bat"
    set VSVERSION="Visual Studio 17 2022"
) ELSE IF EXIST "%ProgramFiles%\Microsoft Visual Studio\2022\Enterprise\Common7\Tools\VsMSBuildCmd.bat" (
    echo "ERROR: Did not find Microsoft Visual Studio"
    EXIT 1
)

where /q msbuild
IF ERRORLEVEL 1 (
    echo "ERROR: Did not find msbuild"
    exit %ERRORLEVEL%
)

where msbuild
msbuild -version
