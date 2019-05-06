#!/bin/bash
#set -o pipefail
#

set -e

script_dir=$(cd "$(dirname "$0")" && pwd)
build_root=$(cd "${script_dir}/../.." && pwd)
log_dir=$build_root
run_e2e_tests=OFF
run_sfc_tests=OFF
run_longhaul_tests=OFF
build_amqp=ON
build_http=ON
build_mqtt=ON
no_blob=OFF
run_unittests=OFF
build_python=OFF
run_valgrind=0
build_folder=$build_root"/cmake/iotsdk_linux"
make=true
toolchainfile=" "
cmake_install_prefix=" "
no_logging=OFF
prov_auth=OFF
prov_use_tpm_simulator=OFF
use_edge_modules=OFF

usage ()
{
    echo "build.sh [options]"
    echo "options"
    echo " -cl, --compileoption <value>  specify a compile option to be passed to gcc"
    echo "   Example: -cl -O1 -cl ..."
    echo " --run-e2e-tests               run the end-to-end tests (e2e tests are skipped by default)"
    echo " --run-sfc-tests               run the end-to-end tests for Service Faults (sfc tests are skipped by default)"
    echo " --run-unittests               run the unit tests"
    echo " --run-longhaul-tests          run long haul tests (long haul tests are not run by default)"
    echo ""
    echo " --no-amqp                     do no build AMQP transport and samples"
    echo " --no-http                     do no build HTTP transport and samples"
    echo " --no-mqtt                     do no build MQTT transport and samples"
    echo " --no_uploadtoblob             do no build upload to blob"
    echo " --no-make                     do not run make after cmake"
    echo " --toolchain-file <file>       pass cmake a toolchain file for cross compiling"
    echo " --install-path-prefix         alternative prefix for make install"
    echo " --build-python <version>      build Python C wrapper module (requires boost) with given python version (2.7 3.4 3.5 are currently supported)"
    echo " -rv, --run_valgrind           will execute ctest with valgrind"
    echo " --no-logging                  Disable logging"
    echo " --provisioning                Use Provisioning with Flow"
    echo " --use-tpm-simulator           Build TPM simulator"
    echo " --use-edge-modules            Build Edge modules"    
    exit 1
}

process_args ()
{
    save_next_arg=0
    extracloptions=" "

    for arg in $*
    do      
      if [ $save_next_arg == 1 ]
      then
        # save arg to pass to gcc
        extracloptions="$arg $extracloptions"
        save_next_arg=0
      elif [ $save_next_arg == 2 ]
      then
        # save arg to pass as toolchain file
        toolchainfile="$arg"
           save_next_arg=0
      elif [ $save_next_arg == 3 ]
      then
        # save the arg to python version
        build_python="$arg"
        if [ $build_python != "2.7" ] && [ $build_python != "3.4" ] && [ $build_python != "3.5" ] && [ $build_python != "3.6" ]
        then
          echo "Supported python versions are 2.7, 3.4 or 3.5 or 3.6"
          exit 1
        fi 
        save_next_arg=0
      elif [ $save_next_arg == 4 ]
      then
        # save arg for install prefix
        cmake_install_prefix="$arg"
        save_next_arg=0
      else
          case "$arg" in
              "-cl" | "--compileoption" ) save_next_arg=1;;
              "--run-e2e-tests" ) run_e2e_tests=ON;;
              "--run-unittests" ) run_unittests=ON;;
              "--run-longhaul-tests" ) run_longhaul_tests=ON;;
              "--no-amqp" ) build_amqp=OFF;;
              "--no-http" ) build_http=OFF;;
              "--no-mqtt" ) build_mqtt=OFF;;
              "--no_uploadtoblob" ) no_blob=ON;;
              "--no-make" ) make=false;;
              "--build-python" ) save_next_arg=3;;
              "--toolchain-file" ) save_next_arg=2;;
              "-rv" | "--run_valgrind" ) run_valgrind=1;;
              "--no-logging" ) no_logging=ON;;
              "--install-path-prefix" ) save_next_arg=4;;
              "--provisioning" ) prov_auth=ON;;
              "--use-tpm-simulator" ) prov_use_tpm_simulator=ON;;
              "--run-sfc-tests" ) run_sfc_tests=ON;;
              "--use-edge-modules") use_edge_modules=ON;;
              * ) usage;;
          esac
      fi
    done

    if [ "$toolchainfile" != " " ]
    then
      toolchainfile=$(readlink -f $toolchainfile)
      toolchainfile="-DCMAKE_TOOLCHAIN_FILE=$toolchainfile"
    fi
   
   if [ "$cmake_install_prefix" != " " ]
   then
     cmake_install_prefix="-DCMAKE_INSTALL_PREFIX=$cmake_install_prefix"
   fi
}

process_args $*

rm -r -f $build_folder
mkdir -p $build_folder
pushd $build_folder
cmake $toolchainfile $cmake_install_prefix -Drun_valgrind:BOOL=$run_valgrind -DcompileOption_C:STRING="$extracloptions" -Drun_e2e_tests:BOOL=$run_e2e_tests -Drun_sfc_tests:BOOL=$run-sfc-tests -Drun_longhaul_tests=$run_longhaul_tests -Duse_amqp:BOOL=$build_amqp -Duse_http:BOOL=$build_http -Duse_mqtt:BOOL=$build_mqtt -Ddont_use_uploadtoblob:BOOL=$no_blob -Drun_unittests:BOOL=$run_unittests -Dbuild_python:STRING=$build_python -Dno_logging:BOOL=$no_logging $build_root -Duse_prov_client:BOOL=$prov_auth -Duse_tpm_simulator:BOOL=$prov_use_tpm_simulator -Duse_edge_modules=$use_edge_modules

if [ "$make" = true ]
then
  # Set the default cores
  MAKE_CORES=$(grep -c ^processor /proc/cpuinfo 2>/dev/null || sysctl -n hw.ncpu)
  
  echo "Initial MAKE_CORES=$MAKE_CORES"
  
  # Make sure there is enough virtual memory on the device to handle more than one job  
  MINVSPACE="1500000"
  
  # Acquire total memory and total swap space setting them to zero in the event the command fails
  MEMAR=( $(sed -n -e 's/^MemTotal:[^0-9]*\([0-9][0-9]*\).*/\1/p' -e 's/^SwapTotal:[^0-9]*\([0-9][0-9]*\).*/\1/p' /proc/meminfo) )
  [ -z "${MEMAR[0]##*[!0-9]*}" ] && MEMAR[0]=0
  [ -z "${MEMAR[1]##*[!0-9]*}" ] && MEMAR[1]=0
  
  let VSPACE=${MEMAR[0]}+${MEMAR[1]}
  
  echo "VSPACE=$VSPACE"

  if [ "$VSPACE" -lt "$MINVSPACE" ] ; then
    echo "WARNING: Not enough space.  Setting MAKE_CORES=1"
    MAKE_CORES=1
  fi
  
  echo "MAKE_CORES=$MAKE_CORES"
  echo "Starting run..."
  date
  make --jobs=$MAKE_CORES
  echo "completed run..."
  date

  # Only for testing E2E behaviour !!! 
  TEST_CORES=16

  if [[ $run_valgrind == 1 ]] ;
  then
    #use doctored openssl
    export LD_LIBRARY_PATH=/usr/local/ssl/lib
    ctest -j $TEST_CORES --output-on-failure
    export LD_LIBRARY_PATH=
  else
    ctest -j $TEST_CORES -C "Debug" --output-on-failure
  fi
fi

popd
