#!/bin/bash -x
# Script to create the makefile to cross compile for host linux-armhf
# Assuming that the variable ARMHF_SYSROOT_DIR points to the system root
# of the host system with requiered libraries and headers.

script_dir=$(cd "$(dirname "$0")" && pwd)
build_root=$(cd "${script_dir}/../.." && pwd)
log_dir=$build_root
run_e2e_tests=OFF
run_longhaul_tests=OFF
build_amqp=ON
build_http=ON
build_mqtt=ON
no_blob=OFF
run_unittests=OFF
build_python=OFF
build_javawrapper=OFF
run_valgrind=0
build_folder=$(pwd)
make=true
toolchainfile="-DCMAKE_TOOLCHAIN_FILE=${script_dir}/cmake_toolchain_file_armhf_linux"
cmake_install_prefix=" "
no_logging=OFF
wip_use_c2d_amqp_methods=OFF

cmake $toolchainfile $cmake_install_prefix -Drun_valgrind:BOOL=$run_valgrind -DcompileOption_C:STRING="$extracloptions" -Drun_e2e_tests:BOOL=$run_e2e_tests -Drun_longhaul_tests=$run_longhaul_tests -Duse_amqp:BOOL=$build_amqp -Duse_http:BOOL=$build_http -Duse_mqtt:BOOL=$build_mqtt -Ddont_use_uploadtoblob:BOOL=$no_blob -Drun_unittests:BOOL=$run_unittests -Dbuild_python:STRING=$build_python -Dbuild_javawrapper:BOOL=$build_javawrapper -Dno_logging:BOOL=$no_logging -Dwip_use_c2d_amqp_methods:BOOL=$wip_use_c2d_amqp_methods $build_root
# Workaround for bad cmake files:
# We need to run cmake twice
cmake $toolchainfile $cmake_install_prefix -Drun_valgrind:BOOL=$run_valgrind -DcompileOption_C:STRING="$extracloptions" -Drun_e2e_tests:BOOL=$run_e2e_tests -Drun_longhaul_tests=$run_longhaul_tests -Duse_amqp:BOOL=$build_amqp -Duse_http:BOOL=$build_http -Duse_mqtt:BOOL=$build_mqtt -Ddont_use_uploadtoblob:BOOL=$no_blob -Drun_unittests:BOOL=$run_unittests -Dbuild_python:STRING=$build_python -Dbuild_javawrapper:BOOL=$build_javawrapper -Dno_logging:BOOL=$no_logging -Dwip_use_c2d_amqp_methods:BOOL=$wip_use_c2d_amqp_methods $build_root
