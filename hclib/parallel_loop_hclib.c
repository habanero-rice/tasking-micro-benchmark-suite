#include "hclib.h"
#include "timing.h"
#include "hclib_stubs.h"

#include <stdio.h>
#include "parallel_loop.h"

void loop_body(void *arg, int index) {
}

void entrypoint(void *arg) {
    int nworkers = hclib_get_num_workers();

    printf("Using %d HClib workers\n", nworkers);

#ifdef HCLIB_MASTER
    loop_domain_t domain;
#else
    hclib_loop_domain_t domain;
#endif
    domain.low = 0;
    domain.high = PARALLEL_LOOP_RANGE;
    domain.stride = 1;
#ifdef HCLIB_MASTER
    domain.tile = (PARALLEL_LOOP_RANGE + nworkers - 1) / nworkers;
#else
    domain.tile = -1;
#endif

    const unsigned long long recursive_start_time = current_time_ns();
    hclib_start_finish();
    {
        hclib_forasync(loop_body, NULL, 1, &domain, FORASYNC_MODE_RECURSIVE);
    }
    hclib_end_finish();
    const unsigned long long recursive_end_time = current_time_ns();
    printf("METRIC recursive_parallel_iters %d %.20f\n", PARALLEL_LOOP_RANGE,
            (double)PARALLEL_LOOP_RANGE / ((double)(recursive_end_time -
                    recursive_start_time) / 1000.0));

    const unsigned long long flat_start_time = current_time_ns();
    hclib_start_finish();
    {
        hclib_forasync(loop_body, NULL, 1, &domain, FORASYNC_MODE_FLAT);
    }
    hclib_end_finish();
    const unsigned long long flat_end_time = current_time_ns();
    printf("METRIC flat_parallel_iters %d %.20f\n", PARALLEL_LOOP_RANGE,
            (double)PARALLEL_LOOP_RANGE / ((double)(flat_end_time -
                    flat_start_time) / 1000.0));
}

int main(int argc, char **argv) {
    hclib_launch(entrypoint, NULL, NULL, 0);
    return 0;
}
