#!/bin/bash -ex

<<<<<<< HEAD
# this should be run from the src directory in the ceph.git (when built with
# automake) or cmake build directory

source $(dirname $0)/detect-build-env-vars.sh

RBD_FEATURES=13 valgrind --tool=memcheck --leak-check=full \
	    --suppressions=${CEPH_ROOT}/src/valgrind.supp unittest_librbd
=======
# this should be run from the src directory in the ceph.git

CEPH_SRC=$(pwd)
export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$CEPH_SRC/.libs"
PATH="$CEPH_SRC:$PATH"

RBD_FEATURES=13 valgrind --tool=memcheck --leak-check=full --suppressions=valgrind.supp unittest_librbd
>>>>>>> upstream/hammer

echo OK
