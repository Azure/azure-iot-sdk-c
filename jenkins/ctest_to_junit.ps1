# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.

# This script will be running on working dir set as ./jenkins
$build_folder = $args[0]
$PSScriptRoot
Push-Location $PSScriptRoot
$xsl = Join-Path -Path (Get-Location) -ChildPath "..\jenkins\ctest_to_junit.xsl" 
$ctest_xml = (Get-ChildItem "../cmake/*/Test.xml" -Recurse).FullName
$junit_xml = Join-Path -Path (Get-Location) -ChildPath "results-junit.xml" 
$xslt = New-Object System.Xml.Xsl.XslCompiledTransform 
$xslt.Load($xsl)
$xslt.Transform($ctest_xml, $junit_xml)

Pop-Location
