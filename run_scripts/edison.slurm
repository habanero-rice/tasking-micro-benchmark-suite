#!/bin/bash -l

#SBATCH -p debug
#SBATCH -N 1
#SBATCH -t 00:05:00
#SBATCH -J hclib-micro-apps
#SBATCH --exclusive
#SBATCH --mail-type=ALL

set -e

ulimit -c unlimited
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

        export HCLIB_WORKERS=24
        export OMP_NUM_THREADS=$HCLIB_WORKERS
        export TBB_NUM_THREADS=$HCLIB_WORKERS
        export CPU_MASK=$(echo "obase=16; $(($((1 << $HCLIB_WORKERS)) - 1))" | bc)

        rm -f default.cfg
        $OCR_INSTALL/share/ocr/scripts/Configs/config-generator.py --threads $HCLIB_WORKERS
        export OCR_CONFIG=$(pwd)/default.cfg

        ARGS=
        if [[ $MODEL == 'realm' ]]; then
            ARGS="-ll:cpu $HCLIB_WORKERS"
        fi

        if [[ -f bin/$EXE ]]; then
            # On Edison the first 12 logical cores are spread evenly across
            # socket 0's cores. The next 12 logical cores are spread evenly
            # across socket 1's cores. Then, the following 12 are back on the
            # same cores of socket 0 via hyperthreading, and then socket 1.
            echo "========== $EXE =========="
            srun --ntasks=1 --ntasks-per-node=1 --cpus-per-task=48 \
                --cpu_bind=verbose,mask_cpu:$CPU_MASK ./bin/$EXE $ARGS &> logs/$MODEL
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
