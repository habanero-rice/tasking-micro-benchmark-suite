#include "hclib.h"
#include "timing.h"

#include <stdio.h>
#include "future_spawn.h"

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

    hclib_start_finish();
    {
        const unsigned long long start_time = current_time_ns();

        hclib_future_t *prev = NULL;
        int nlaunched = 0;
        do {
            prev = hclib_async_future(empty_task, NULL, &prev, 1, NULL);
            nlaunched++;
        } while (nlaunched < NFUTURES);

        const unsigned long long end_time = current_time_ns();
        printf("METRIC future_create %d %.20f\n", NFUTURES,
                (double)NFUTURES / ((double)(end_time - start_time) / 1000.0));
    }
    hclib_end_finish();

    const unsigned long long start_time = current_time_ns();
    hclib_start_finish();
    {
        hclib_future_t *prev = NULL;
        int nlaunched = 0;
        do {
            prev = hclib_async_future(empty_task, NULL, &prev, 1, NULL);
            nlaunched++;
        } while (nlaunched < NFUTURES);
    }
    hclib_end_finish();
    const unsigned long long end_time = current_time_ns();
    printf("METRIC future_run %d %.20f\n", NFUTURES,
            (double)NFUTURES / ((double)(end_time - start_time) / 1000.0));
}

int main(int argc, char **argv) {
    hclib_launch(entrypoint, NULL, NULL, 0);
    return 0;
}
