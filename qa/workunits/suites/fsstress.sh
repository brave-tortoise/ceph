#!/bin/bash

<<<<<<< HEAD
BIN_PATH=${TESTDIR}/fsstress/ltp-full-20091231/testcases/kernel/fs/fsstress/fsstress
=======
if [ ! -f /usr/lib/ltp/testcases/bin/fsstress ]
then
    path=`pwd`
    mkdir -p /tmp/fsstress
    cd /tmp/fsstress
    wget -q -O /tmp/fsstress/ltp-full.tgz http://download.ceph.com/qa/ltp-full-20091231.tgz
    tar xzf /tmp/fsstress/ltp-full.tgz
    rm /tmp/fsstress/ltp-full.tgz
    cd /tmp/fsstress/ltp-full-20091231/testcases/kernel/fs/fsstress
    make
    sudo mkdir -p /usr/lib/ltp/testcases/bin
    sudo cp -av --remove-destination /tmp/fsstress/ltp-full-20091231/testcases/kernel/fs/fsstress/fsstress /usr/lib/ltp/testcases/bin/fsstress
    sudo chmod 755 /usr/lib/ltp/testcases/bin/fsstress
    rm -Rf /tmp/fsstress
    cd $path
fi
>>>>>>> upstream/hammer

path=`pwd`
trap "rm -rf ${TESTDIR}/fsstress" EXIT
mkdir -p ${TESTDIR}/fsstress
cd ${TESTDIR}/fsstress
wget -q -O ${TESTDIR}/fsstress/ltp-full.tgz http://download.ceph.com/qa/ltp-full-20091231.tgz
tar xzf ${TESTDIR}/fsstress/ltp-full.tgz
rm ${TESTDIR}/fsstress/ltp-full.tgz
cd ${TESTDIR}/fsstress/ltp-full-20091231/testcases/kernel/fs/fsstress
make
cd $path

command="${BIN_PATH} -d fsstress-`hostname`$$ -l 1 -n 1000 -p 10 -v"

echo "Starting fsstress $command"
mkdir fsstress`hostname`-$$
$command
