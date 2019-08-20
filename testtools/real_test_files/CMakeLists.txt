#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

#this is CMakeLists for real_test.  Do not attempt to build these into a library, since we don't know
#ahead of time which real_* a given UT will want to use versus which they'll want to mock themselves.

set(IOTHUB_TEST_REAL_INC_FOLDER ${CMAKE_CURRENT_LIST_DIR}/inc CACHE INTERNAL "this is what needs to be included if using real_test" FORCE)
set(IOTHUB_TEST_REAL_SRC_FOLDER ${CMAKE_CURRENT_LIST_DIR}/src CACHE INTERNAL "this is what needs to be set for source directory if using real_test" FORCE)

