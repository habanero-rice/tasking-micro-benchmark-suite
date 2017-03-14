#include "qthread.h"
#include "timing.h"

#include <stdio.h>
#include <assert.h>
#include "future_spawn.h"

aligned_t empty_task(void *arg) {
    /*
     * Unfortunately need to put something here to compare against OpenMP tasks,
     * otherwise some OpenMP compilers will make the task a no-op.
     */
    int incr = 0;
    incr = incr + 1;
    return incr;
}

void entrypoint(void *arg) {
    int status = qthread_initialize();
    assert(status == QTHREAD_SUCCESS);
    int nworkers = qthread_num_workers();
    int i;
    // Used for 1) waiting the completion of each task and 2) getting a future value
    aligned_t return_values1[NFUTURES];
    aligned_t return_values2[NFUTURES];

    printf("Using %d Qthread workers\n", nworkers);

    {
	const unsigned long long start_time = current_time_ns();

	int nlaunched = 0;
	do {
	    qthread_fork(empty_task, NULL, &return_values1[nlaunched]);

	    nlaunched++;
	} while (nlaunched < NFUTURES);    

	const unsigned long long end_time = current_time_ns();
        printf("METRIC future_create %d %.20f\n", NFUTURES,
                (double)NFUTURES / ((double)(end_time - start_time) / 1000.0));
    }
    // Waiting for the completion of all the tasks
    for (i = 0; i < NFUTURES; i++) {
	// blocked until task the i-th task is complete
	qthread_readFF(NULL, &return_values1[i]);
    }

    const unsigned long long start_time = current_time_ns();
    {
	int nlaunched = 0;
	do {
	    qthread_fork(empty_task, NULL, &return_values2[nlaunched]);

	    nlaunched++;
	} while (nlaunched < NFUTURES);
	// Waiting for the completion of all the tasks
	for (i = 0; i < NFUTURES; i++) {
	    // blocked until task the i-th task is complete 
	    qthread_readFF(NULL, &return_values2[i]);
	}
    }
    const unsigned long long end_time = current_time_ns();
    printf("METRIC future_run %d %.20f\n", NFUTURES,
            (double)NFUTURES / ((double)(end_time - start_time) / 1000.0));

    qthread_finalize();    
}

int main(int argc, char **argv) {
    entrypoint(NULL);
    return 0;
}
