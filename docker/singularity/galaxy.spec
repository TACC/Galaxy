# run with `singularity bootstrap <container>`
Bootstrap: docker
From: ubuntu:bionic

%post
  mkdir /home1 /work /scratch /gpfs /corral-repl /corral-tacc /data

########################################
# Install mpi
########################################

# necessities and IB stack
apt-get update \
    && apt-get install -yq --no-install-recommends \
        ca-certificates build-essential curl gfortran bison \
        rdma-core \
        libxml2-dev \
        libnuma-dev

# Install PSM2
export PSM=PSM2
export PSMV=11.2.78
export PSMD=opa-psm2-${PSM}_${PSMV}

curl -L https://github.com/intel/opa-psm2/archive/${PSM}_${PSMV}.tar.gz | tar -xzf - \
    && cd ${PSMD} \
    && make PSM_AVX=1 -j $(nproc --all 2>/dev/null || echo 2) \
    && make LIBDIR=/usr/lib/x86_64-linux-gnu install \
    && cd ../ && rm -rf ${PSMD}

# Install mvapich2-2.3
export MAJV=2
export MINV=3
export DIR=mvapich${MAJV}-${MAJV}.${MINV}

curl http://mvapich.cse.ohio-state.edu/download/mvapich/mv${MAJV}/${DIR}.tar.gz | tar -xzf - \
    && cd ${DIR} \
    && ./configure --with-device=ch3:psm \
    && make -j $(nproc --all 2>/dev/null || echo 2) && make install \
    && mpicc examples/hellow.c -o /usr/bin/hellow \
    && cd ../ && rm -rf ${DIR} && rm -rf /usr/local/share/doc/mvapich2
