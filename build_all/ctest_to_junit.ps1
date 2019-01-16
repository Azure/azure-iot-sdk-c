# Copyright (c) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE file in the project root for full license information.

# This script will be running on working dir set as build_all  
$xsl = "./ctest_to_junit.xsl" 
$ctest_xml = (Get-ChildItem "./Testing/*/Test.xml").FullName 
$junit_xml = "./Testing/results-junit.xml" 
$xslt = New-Object System.Xml.Xsl.XslCompiledTransform 
$xslt.Load($xsl) 
$xslt.Transform($ctest_xml, $junit_xml) 