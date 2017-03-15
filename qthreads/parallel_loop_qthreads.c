#include "qthread.h"
#include "qthread/qloop.h"
#include "timing.h"

#include <stdio.h>
#include <assert.h>
#include "parallel_loop.h"

void loop_body(const size_t startat, const size_t stopat, void *arg) {
}

void entrypoint(void *arg) {
    int status = qthread_initialize();
    assert(status == QTHREAD_SUCCESS);
    int nworkers = qthread_num_workers();

    printf("Using %d Qthread workers\n", nworkers);

    const unsigned long long recursive_start_time = current_time_ns();
    {
	// A tree-based forall implementation
	qt_loop(0, PARALLEL_LOOP_RANGE, loop_body, NULL);	
    }
    const unsigned long long recursive_end_time = current_time_ns();
    printf("METRIC recursive_parallel_iters %d %.20f\n", PARALLEL_LOOP_RANGE,
            (double)PARALLEL_LOOP_RANGE / ((double)(recursive_end_time -
                    recursive_start_time) / 1000.0));

    const unsigned long long flat_start_time = current_time_ns();

    {
    }

    const unsigned long long flat_end_time = current_time_ns();
    printf("METRIC flat_parallel_iters %d %.20f\n", PARALLEL_LOOP_RANGE,
            (double)PARALLEL_LOOP_RANGE / ((double)(flat_end_time -
                    flat_start_time) / 1000.0));

    
    qthread_finalize();
}

int main(int argc, char **argv) {
    entrypoint(NULL);
    return 0;
}
