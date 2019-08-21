#!/bin/bash
## ========================================================================== ##
## Copyright (c) 2014-2019 The University of Texas at Austin.                 ##
## All rights reserved.                                                       ##
##                                                                            ##
## Licensed under the Apache License, Version 2.0 (the "License");            ##
## you may not use this file except in compliance with the License.           ##
## A copy of the License is included with this software in the file LICENSE.  ##
## If your copy does not contain the License, you may obtain a copy of the    ##
## License at:                                                                ##
##                                                                            ##
##     https://www.apache.org/licenses/LICENSE-2.0                            ##
##                                                                            ##
## Unless required by applicable law or agreed to in writing, software        ##
## distributed under the License is distributed on an "AS IS" BASIS, WITHOUT  ##
## WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.           ##
## See the License for the specific language governing permissions and        ##
## limitations under the License.                                             ##
##                                                                            ##
## ========================================================================== ##


if [ $1 ]; then
  echo "usage: unit-tests.sh"
  echo "This script runs Galaxy unit tests (requires GXY_UNIT_TESTING build flag)"
  exit 1
fi

function report
{
  echo "GALAXY: $1"
}

function fail
{
  echo "GALAXY: ERROR - $1"
  exit 1
}

function run_tests()
{
  cd ${GXY_TESTS}
  for module in *; do
    report "  Running unit tests for module ${module}"
    cd ${module}
    for test in *; do
      TESTS=$((${TESTS} + 1))
      report "    testing ${test}..."
      ${MPI_COMMAND} ./${test} #> /dev/null 2>&1
      if [ $? == 0 ]; then
        report "    test passed: ${test}"
      else
        report "    test FAILED ($?): ${test} ====="
        FAILS=$((${FAILS} + 1))
      fi
    done
    report "  Completed unit tests for module ${module}"
    cd ..
  done
  cd ${GXY_ROOT}
}


#######################
#                     #
#  begin main script  #
#                     #
#######################

GXY_ROOT=$PWD

if [ -f ${GXY_ROOT}/unit-tests.sh ]; then
  # already in test dir, help a user out
  GXY_ROOT=${PWD}/..
fi

GXY_TESTS=${GXY_ROOT}/install/tests
GXY_ENV=${GXY_ROOT}/install/galaxy.env

if [ ! -d ${GXY_TESTS} ]; then
  fail "Could not find Galaxy unit tests at '${GXY_TESTS}'. Please build Galaxy with GXY_UNIT_TESTING set ON."
fi
if [ ! -f ${GXY_ENV} ]; then
  fail "Could not find Galaxy environment file at '${GXY_ENV}'"
fi

report "Sourcing Galaxy environment"
. ${GXY_ENV}

# reasonable settings for Travis-CI VM environment
export GXY_APP_NTHREADS=1
export GXY_NTHREADS=4
TESTS=0
FAILS=0

MPI_COMMAND=""
report "Running single process tests..."
run_tests

if [ "${TRAVIS_OS_NAME}" == "linux" ]; then
  MPI_COMMAND="mpirun -np 2"
  report "Running MPI tests with command '${MPI_COMMAND}'..."
  run_tests
fi

if [ ${FAILS} == 0 ]; then
  report "${TESTS}/${TESTS} unit tests passed"
else
  fail "${FAILS}/${TESTS} unit tests FAILED"
fi

report "done!"
cd ${GXY_ROOT}
exit 0
