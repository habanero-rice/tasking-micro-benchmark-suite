#include "timing.h"

#include <omp.h>
#include <stdio.h>
#include "fan_out.h"

int main(int argc, char **argv) {
    int nthreads;
#pragma omp parallel default(none) shared(nthreads)
#pragma omp master
    {
        nthreads = omp_get_num_threads();
    }
    printf("Using %d OpenMP threads\n", nthreads);

#pragma omp parallel default(none)
#pragma omp master
    {
        int dep_arr[1];
        dep_arr[0] = dep_arr[0]; // To disable unused variable warnings

        const unsigned long long start_time = current_time_ns();

#pragma omp taskgroup
        {
            int incr = 0;

#pragma omp task default(none) firstprivate(incr) depend(out:dep_arr[0])
            {
                incr = incr + 1;
            }

            int i;
            for (i = 0; i < FAN_OUT; i++) {
#pragma omp task default(none) firstprivate(incr) depend(in:dep_arr[0])
                {
                    incr = incr + 1;
                }
            }
        }

        const unsigned long long end_time = current_time_ns();
        printf("METRIC fan_out %d %.20f\n", FAN_OUT,
                (double)FAN_OUT / ((double)(end_time - start_time) / 1000.0));
    }

    return 0;
}
