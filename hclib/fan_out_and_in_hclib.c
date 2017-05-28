#include "hclib.h"
#include "timing.h"
#include "hclib_stubs.h"

#include <stdio.h>
#include "fan_out_and_in.h"

#ifndef MAX_NUM_WAITS
#define MAX_NUM_WAITS 4
#endif

void *empty_task(void *arg) {
    /*
     * Unfortunately need to put something here to compare against OpenMP tasks,
     * otherwise some OpenMP compilers will make the task a no-op.
     */
    int incr = 0;
    incr = incr + 1;
    return NULL;
}

void entrypoint(void *arg) {
    int nworkers = hclib_get_num_workers();

    printf("Using %d HClib workers\n", nworkers);

    hclib_promise_t prom;
    hclib_promise_init(&prom);

#ifdef HCLIB_MASTER
    hclib_future_t *fut[2] = { NULL, NULL };
    fut[0] = hclib_get_future_for_promise(&prom);
#else
    hclib_future_t *fut = hclib_get_future_for_promise(&prom);
#endif
    hclib_future_t **futures = (hclib_future_t **)malloc(
            FAN_OUT_AND_IN * sizeof(hclib_future_t *));
    assert(futures);

    const unsigned long long start_time = current_time_ns();

    hclib_promise_put(&prom, NULL);

    int i;
    for (i = 0; i < FAN_OUT_AND_IN; i++) {
#ifdef HCLIB_MASTER
        futures[i] = hclib_async_future(empty_task, NULL, fut, NULL, NULL, 0);
#else
        futures[i] = hclib_async_future(empty_task, NULL, &fut, 1, NULL);
#endif
    }

#ifdef HCLIB_MASTER
    hclib_future_t *awaits[MAX_NUM_WAITS + 1];
#endif
    int nfutures = FAN_OUT_AND_IN;
    while (nfutures > 1) {
        int next_nfutures = 0;

        for (i = 0; i < nfutures; i += MAX_NUM_WAITS) {
            int this_n_futures = nfutures - i;
            if (this_n_futures > MAX_NUM_WAITS) this_n_futures = MAX_NUM_WAITS;

#ifdef HCLIB_MASTER
            memcpy(awaits, futures + i, this_n_futures * sizeof(hclib_future_t *));
            awaits[this_n_futures] = NULL;

            futures[next_nfutures] = hclib_async_future(empty_task, NULL,
                    awaits, NULL, NULL, 0);
#else
            futures[next_nfutures] = hclib_async_future(empty_task, NULL,
                    futures + i, this_n_futures, NULL);
#endif
            next_nfutures++;
        }
        nfutures = next_nfutures;
    }

    hclib_future_wait(futures[0]);
    const unsigned long long end_time = current_time_ns();
    printf("METRIC fan_out_and_in %d %.20f\n", FAN_OUT_AND_IN,
            (double)FAN_OUT_AND_IN / ((double)(end_time - start_time) /
                1000.0));
}

int main(int argc, char **argv) {
    hclib_launch(entrypoint, NULL, NULL, 0);
    return 0;
}
