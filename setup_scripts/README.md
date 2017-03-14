### Building Qthreads
The provided setup_qthreads.sh will automatically get, configure, and build the Qthreads library (http://www.cs.sandia.gov/qthreads/) with hwloc (https://www.open-mpi.org/projects/hwloc/). It will create qthreads and hwloc-1.11.5 directories within the current directory. Two different task schedulers (nemesis and sherwood) will be built under the qthreads directory. QTHREADS_HOME must be set to either qthreads/nemesis-install or qthreads/sherwood-install. Also, HWLOC_HOME must be set to hwloc-1.11.5/install.

