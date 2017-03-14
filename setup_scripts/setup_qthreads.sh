#!/bin/bash -xe

BASEDIR=$PWD
HWLOCDIR=hwloc-1.11.5
QTHREADSDIR=qthreads

if ! test -d ${HWLOCDIR}; then
    # We have to get 1.11.5 because 1.11.6 is incompatible with qthreads
    wget --no-check-certificate https://www.open-mpi.org/software/hwloc/v1.11/downloads/hwloc-1.11.5.tar.gz
    tar xvfz hwloc-1.11.5.tar.gz

    # using the same flags as Chapel does
    HCONFIGFLAGS="--enable-static --disable-shared --disable-cairo --disable-libxml2 --disable-libudev --disable-libnuma --disable-opencl --disable-pci"

    cd ${HWLOCDIR}
    if ! test -e configure; then
	./autogen.sh
    fi
    mkdir -p build
    cd build 
    ../configure ${HCONFIGFLAGS} --prefix=${BASEDIR}/${HWLOCDIR}/install
    make && make install
    cd -

    cd ${BASEDIR}
fi

if ! test -d ${QTHREADSDIR}; then 
    git clone https://github.com/Qthreads/qthreads

    # using the same flags as Chapel does (except for schedulers)
    QCONFIGFLAGS="--enable-guard-pages --enable-static --disable-shared --disable-spawn-cache --enable-condwait-queue --with-topology=hwloc --with-hwloc=${BASEDIR}/${HWLOCDIR}/install"

    cd ${QTHREADSDIR}
    if ! test -e configure; then
	./autogen.sh
    fi

    # building nemesis scheduler
    if ! test -d build_nemesis; then
	mkdir -p build_nemesis
	cd build_nemesis
	../configure ${QCONFIGFLAGS} --with-scheduler=nemesis --prefix=${BASEDIR}/${QTHREADSDIR}/nemesis_install
	make && make install
	cd -
    fi

    # buidling sherwood scheduler
    if ! test -d build_sherwood; then
	mkdir -p build_sherwood
	cd build_sherwood
	../configure ${QCONFIGFLAGS} --with-scheduler=sherwood --prefix=${BASEDIR}/${QTHREADSDIR}/sherwood_install
	make && make install
	cd -
    fi
    
    cd ${BASEDIR}
fi

