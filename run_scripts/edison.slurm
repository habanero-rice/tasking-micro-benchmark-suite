#!/bin/bash -l

#SBATCH -p debug
#SBATCH -N 1
#SBATCH -t 00:20:00
#SBATCH -J hclib-micro-apps
#SBATCH --exclusive
#SBATCH --mail-type=ALL

set -e

ulimit -c unlimited
export HCLIB_WORKERS=24
export OMP_NUM_THREADS=$HCLIB_WORKERS
export TBB_NUM_THREADS=$HCLIB_WORKERS
export MODELS="hclib iomp gomp tbb ocr realm"

# 2 sockets x 12-core CPUs

TASKING_SUITE_HOME=$HOME/tasking-micro-benchmark-suite
cd $TASKING_SUITE_HOME

make clean
CXX=g++ CC=gcc make omp -j
TMP_DIR=$(mktemp -d)
mv bin/*_omp $TMP_DIR/

make clean
CXX=icpc CC=icc make -j
pushd bin
for F in $(ls *_omp); do mv $F ${F:0:$((${#F} - 4))}_iomp; done
popd
pushd $TMP_DIR
for F in $(ls *_omp); do mv $F $TASKING_SUITE_HOME/bin/${F:0:$((${#F} - 4))}_gomp; done
popd
rm -r $TMP_DIR

srun --ntasks=1 --ntasks-per-node=1 --cpus-per-task=24 cat /proc/cpuinfo

LOG_FILE=metrics.csv
rm -f $LOG_FILE

LINE="metric,dataset"
for MODEL in $MODELS; do
    LINE="$LINE,$MODEL"
done
LINE="$LINE,,"
echo $LINE >> $LOG_FILE

# for TEST in task_spawn future_spawn task_wait_flat task_wait_recursive fan_out \
#         fan_out_and_in bin_fan_out parallel_loop prod_cons reduce_var \
#         unbalanced_bin_fan_out; do
for TEST in task_spawn; do
    echo
    echo "========== Testing ${TEST} =========="
    echo

    for MODEL in $MODELS; do
        EXE=${TEST}_${MODEL}
        ARGS=
        if [[ $MODEL == 'realm' ]]; then
            ARGS="-ll:cpu 24"
        fi

        if [[ -f bin/$EXE ]]; then
            echo "========== $EXE =========="
            srun --ntasks=1 --ntasks-per-node=1 --cpus-per-task=24 \
                --cpu_bind=verbose,sockets --exclusive ./bin/$EXE $ARGS &> logs/$MODEL
                # --exclusive ./bin/$EXE $ARGS &> logs/$MODEL
        fi
    done

    UNIQUE_METRICS=$(cat logs/hclib | grep "^METRIC " | awk '{ print $2 }' | sort | \
        uniq)
    for METRIC in $UNIQUE_METRICS; do
        DATASET=$(cat logs/hclib | grep "^METRIC $METRIC " | head -n 1 | \
            awk '{ print $3 }')

        LINE="$METRIC,$DATASET"
        for MODEL in $MODELS; do
            if [[ -f logs/$MODEL ]]; then
                PERF=$(cat logs/$MODEL | grep "^METRIC $METRIC" | \
                    awk '{ print $4 }' | sort -n -r | head -n 1)
                LINE="$LINE,$PERF"
            else
                LINE="$LINE,"
            fi
        done
        LINE="$LINE,,"
        echo $LINE >> $LOG_FILE
    done

#     # Cleanup log files
#     for MODEL in $MODELS; do
#         EXE=${TEST}_${MODEL}
#         if [[ -f ./bin/$EXE ]]; then
#             rm logs/$MODEL
#         fi
#     done
done
