#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#this is CMakeLists.txt for blob_ut
cmake_minimum_required (VERSION 3.5)

compileAsC99()

# Explicitly override MAX_BLOCK_COUNT for upload tests as the default of 50,000 is too large for UT to run quickly in
add_definitions(-DGB_MEASURE_MEMORY_FOR_THIS -DGB_DEBUG_ALLOC -DMAX_BLOCK_COUNT=100)

set(theseTestsName blob_ut )

generate_cppunittest_wrapper(${theseTestsName})

set(${theseTestsName}_c_files
    ../../src/blob.c
)

set(${theseTestsName}_h_files
)

build_c_test_artifacts(${theseTestsName} OFF "tests/azure_iothub_client_tests")
