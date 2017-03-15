#include "qthread.h"
#include "timing.h"

#include <stdio.h>
#include <assert.h>
#include "bin_fan_out.h"

aligned_t donecount = 0;

aligned_t recurse(void *arg) {
    const size_t depth = (size_t)arg;

    if (depth < BIN_FAN_OUT_DEPTH) {
        qthread_fork(recurse, (void *)(depth + 1), NULL);
        qthread_fork(recurse, (void *)(depth + 1), NULL);
    } else {
	qthread_incr(&donecount, 1);
    }
    return 0;
}

void entrypoint(void *arg) {
    int status = qthread_initialize();
    assert(status == QTHREAD_SUCCESS);
    int nworkers = qthread_num_workers();    
    printf("Using %d Qthread workers\n", nworkers);

    const unsigned long long start_time = current_time_ns();
    {
	recurse(0);
    }
    while (donecount != (1 << BIN_FAN_OUT_DEPTH)) {
	qthread_yield();
    }
    const unsigned long long end_time = current_time_ns();
    printf("METRIC bin_fan_out %d %.20f\n", BIN_FAN_OUT_DEPTH,
            (double)(1 << BIN_FAN_OUT_DEPTH) /
            ((double)(end_time - start_time) / 1000.0));
}

int main(int argc, char **argv) {
    entrypoint(NULL);
    return 0;
}
