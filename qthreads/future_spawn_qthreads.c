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
    // Used for 1) waiting the completion of each task and 2) getting a future value
    aligned_t return_values1[NFUTURES+1];
    aligned_t return_values2[NFUTURES+1];
    
    printf("Using %d Qthread workers\n", nworkers);

    {
	const unsigned long long start_time = current_time_ns();

	int nlaunched = 0;
	qthread_fill(&return_values1[0]);
	do {
	    qthread_fork_precond(empty_task, NULL, &return_values1[nlaunched+1], 1, &return_values1[nlaunched]);

	    nlaunched++;
	} while (nlaunched < NFUTURES);    

	const unsigned long long end_time = current_time_ns();
        printf("METRIC future_create %d %.20f\n", NFUTURES,
                (double)NFUTURES / ((double)(end_time - start_time) / 1000.0));
    }
    // waiting for the completion of the last task
    qthread_readFF(NULL, &return_values1[NFUTURES]);

    int i;
    // Just in case
    for (i = 0; i < NFUTURES+1; i++) {
	// blocked until task the i-th task is complete 
	qthread_readFF(NULL, &return_values1[i]);
    }
    
    const unsigned long long start_time = current_time_ns();
    {
	int nlaunched = 0;
	qthread_fill(&return_values2[0]);
	do {
	    qthread_fork_precond(empty_task, NULL, &return_values2[nlaunched+1], 1, &return_values2[nlaunched]);

	    nlaunched++;
	} while (nlaunched < NFUTURES);
	// Waiting for the completion of the last task
	qthread_readFF(NULL, &return_values2[NFUTURES]);
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
