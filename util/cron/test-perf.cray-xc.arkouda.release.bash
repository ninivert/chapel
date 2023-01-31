#!/usr/bin/env bash
#
# Run arkouda testing on a cray-xc

CWD=$(cd $(dirname $0) ; pwd)

export CHPL_TEST_PERF_CONFIG_NAME='16-node-xc'
export CHPL_NIGHTLY_TEST_CONFIG_NAME="perf.cray-xc.arkouda.release"
export CHPL_TEST_SYNC_ARKOUDA_GRAPHS="false"

# setup arkouda
source $CWD/common-arkouda.bash
export ARKOUDA_NUMLOCALES=16

module list

# setup for XC perf (ugni, gnu, 28-core broadwell)
module unload $(module -t list 2>&1 | grep PrgEnv-)
module load PrgEnv-gnu
# pin gcc 11.2.0 due to warnings with 12.1.0
module swap gcc gcc/11.2.0
module unload $(module -t list 2>&1 | grep craype-hugepages)
module load craype-hugepages16M
module unload perftools-base
module unload atp
module load craype-sandybridge # use craype-x86-cascadelake with 1.26

module list

export CHPL_LAUNCHER_CONSTRAINT="CL48,192GB"
export CHPL_LAUNCHER_CORES_PER_LOCALE=96
export CHPL_LAUNCHER=slurm-srun
nightly_args="${nightly_args} -no-buildcheck"

test_release
sync_graphs
