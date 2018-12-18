#!/bin/bash
## ========================================================================== ##
## Copyright (c) 2014-2018 The University of Texas at Austin.                 ##
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
  echo "usage: image-gold-tests.sh"
  echo "This script will generate a radial dataset, render it with Galaxy, and compare the results against the golden images in this directory."
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

GXY_ROOT=$PWD

if [ -f ${GXY_ROOT}/image-gold-tests.sh ]; then
  # already in test dir, help a user out
  GXY_ROOT=${PWD}/..
elif [ -d ${GXY_ROOT}/tests ]; then
  # in root dir, move to tests
  cd ${GXY_ROOT}/tests
else 
  fail "could not find tests dir from PWD: $PWD"
fi


GXY_BIN=${GXY_ROOT}/install/bin
GXY_RADIAL=${GXY_BIN}/radial
GXY_VTI2VOL=${GXY_BIN}/vti2vol
GXY_IMAGE_WRITER=${GXY_BIN}/vis
GXY_ENV=${GXY_ROOT}/install/galaxy.env
PERCEPTUAL_DIFF=`which perceptualdiff`

if [ ! -x ${GXY_RADIAL} ]; then
  fail "Could not find or execute the Galaxy radial generator '${GXY_RADIAL}'"
fi
if [ ! -x ${GXY_VTI2VOL} ]; then
  fail "Could not find or execute the Galaxy vti2vol '${GXY_VTI2VOL}'"
fi
if [ ! -x ${GXY_IMAGE_WRITER} ]; then
  fail "Could not find or execute the Galaxy image writer '${GXY_IMAGE_WRITER}'. Ensure Galaxy was configured with -D GXY_WRITE_IMAGES=ON"
fi
if [ ! -f ${GXY_ENV} ]; then
  fail "Could not find Galaxy environment file at '${GXY_ENV}'"
fi
if [ -z ${PERCEPTUAL_DIFF} ]; then
  fail "could not find perceptualdiff"
fi

report "Sourcing Galaxy environment"
. ${GXY_ENV}
# reasonable settings for Travis-CI VM environment
export GXY_APP_NTHREADS=1
export GXY_NTHREADS=4
RESOLUTION="-s 512 512"
PDIFF_OPTIONS="-fov 85"
MPI_COMMAND="mpirun -n 2"

report "Generating radial-0.vti with ${GXY_RADIAL}"
${GXY_RADIAL} -r 256 256 256 > /dev/null 2>&1
if [ $? != 0 ]; then
  fail "$GXY_RADIAL exited with code $?"
fi

GXY_VOLS="oneBall eightBalls xramp yramp zramp"
report "Converting vti to vol with ${GXY_VTI2VOL}"
${GXY_VTI2VOL} radial-0.vti ${GXY_VOLS} > /dev/null 2>&1
if [ $? != 0 ]; then
  fail "$GXY_VTI2VOL exited with code $?"
fi

TESTS=0
FAILS=0
for do_mpi in 0 1; do
  for state in *.state; do
    test=$(echo ${state} | sed s/\.state//)
    report "Generating ${test} images"
    if [ ${do_mpi} == 0 ]; then
      report "  using single process"
      MPI=""
    else
      report "  using multiprocess with MPI command '${MPI_COMMAND}'"
      export GXY_NTHREADS=1
      MPI=${MPI_COMMAND}
    fi
    ${MPI} ${GXY_IMAGE_WRITER} ${RESOLUTION} ${state} #> /dev/null 2>&1
    if [ $? != 0 ]; then
      fail "$GXY_IMAGE_WRITER exited with code $?"
    fi

    report "Comparing generated images with golds"
    for image in image*png; do
      TESTS=$((${TESTS} + 1))
      GOLD=golds/$(echo ${image} | sed s/image/${test}/)
      report "  comparing ${image} to ${GOLD}"
      ${PERCEPTUAL_DIFF} ${image} ${GOLD} ${PDIFF_OPTIONS}
      if [ $? == 0 ]; then
        report "    test passed: ${image} ${GOLD}"
      else
        report "    test FAILED ($?): ${image} ${GOLD} ====="
        FAILS=$((${FAILS} + 1))
      fi
    done
    report "Cleaning up ${test} images"
    rm -f image*png
  done
done

if [ ${FAILS} == 0 ]; then
  report "${TESTS}/${TESTS} image comparison tests passed"
else
  fail "${FAILS}/${TESTS} image comparisons failed"
fi

report "done!"
cd ${GXY_ROOT}
exit 0
