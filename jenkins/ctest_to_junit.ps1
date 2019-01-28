# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.

Set-PSDebug -Trace 2
# This script will be running on working dir set as build_all  
$PSScriptRoot
Push-Location $PSScriptRoot
$outfile = Join-Path -Path (Get-Location) -ChildPath "results-junit.xml"
$xsl = Join-Path -Path (Get-Location) -ChildPath "..\jenkins\ctest_to_junit.xsl" 
$ctest_xml = (Get-ChildItem "../cmake/$args[0]/*/Test.xml" -Recurse).FullName
$junit_xml = Join-Path -Path (Get-Location) -ChildPath "results-junit.xml" 
$xslt = New-Object System.Xml.Xsl.XslCompiledTransform 
$xslt.Load($xsl) 
$xslt.Transform($ctest_xml, $junit_xml)

Pop-Location
