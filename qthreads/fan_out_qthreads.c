#include "qthread.h"
#include "timing.h"

#include <stdio.h>
#include <assert.h>
#include "fan_out.h"

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
    aligned_t rets[FAN_OUT];
    
    printf("Using %d Qthread workers\n", nworkers);
    
    const unsigned long long start_time = current_time_ns();
    aligned_t cond;
    qthread_empty(&cond);    
    {
        int i;
        for (i = 0; i < FAN_OUT; i++) {
	    qthread_fork_precond(empty_task, NULL, &rets[i], 1, &cond);
        }
	qthread_fill(&cond);
	for (i = 0; i < FAN_OUT; i++) {
	    qthread_readFF(NULL, &rets[i]);
	}	
    }   
    const unsigned long long end_time = current_time_ns();
    printf("METRIC fan_out %d %.20f\n", FAN_OUT,
            (double)FAN_OUT / ((double)(end_time - start_time) / 1000.0));
}

int main(int argc, char **argv) {
    entrypoint(NULL);
    return 0;
}
