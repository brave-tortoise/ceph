#!/bin/sh -ex

relpath=$(dirname $0)/../../../src/test/pybind
<<<<<<< HEAD

if [ -n "${VALGRIND}" ]; then
  valgrind --tool=${VALGRIND} --suppressions=${TESTDIR}/valgrind.supp \
    nosetests -v $relpath/test_rbd.py
else
  nosetests -v $relpath/test_rbd.py
fi
=======
nosetests -v $relpath/test_rbd.py
>>>>>>> upstream/hammer
exit 0
