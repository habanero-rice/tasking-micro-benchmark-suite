#include <Kokkos_Macros.hpp>
#include <Kokkos_Core.hpp>
#include "Kokkos_HostSpace.hpp"
#include "Kokkos_TaskScheduler.hpp"
#include "Kokkos_TaskPolicy.hpp"

#include "timing.h"
#include <stdio.h>
#include "parallel_loop.h"


template< typename Space >
struct BodyTask
{
    int niters;
    typedef void                            value_type;

    KOKKOS_INLINE_FUNCTION
    BodyTask(int setIters) : niters(setIters) { }

    KOKKOS_INLINE_FUNCTION
    void operator()( typename Kokkos::TaskScheduler< Space >::member_type &) {
        for (int i = 0; i < niters; i++) {
            ;
        }
    }
};

template< typename Space >
struct DriverTask
{
    typedef void                            value_type;

    Kokkos::TaskScheduler< Space >  sched;
    int nchunks;
    std::vector< Kokkos::Future< void, Space > > futures;
    int respawns = 0;
    unsigned long long start_time;

    KOKKOS_INLINE_FUNCTION
    DriverTask( const Kokkos::TaskScheduler< Space > & set_sched,
            int nthreads) : sched(set_sched), nchunks(2 * nthreads) {
    }

    KOKKOS_INLINE_FUNCTION
    void operator()( typename Kokkos::TaskScheduler< Space >::member_type &) {
        respawns++;

        if (respawns == 1) {
            // Measure task creation
            start_time = current_time_ns();

            // Make some reasonable guess at a good chunk size...
            int chunkSize = (PARALLEL_LOOP_RANGE + nchunks - 1) / nchunks;

            for (int launched = 0; launched < nchunks; launched++) {
                int chunkStart = launched * chunkSize;
                int chunkEnd = (launched + 1) * chunkSize;
                if (chunkEnd > PARALLEL_LOOP_RANGE) {
                    chunkEnd = PARALLEL_LOOP_RANGE;
                }
                int niters = chunkEnd - chunkStart;

                futures.push_back(Kokkos::task_spawn(Kokkos::TaskSingle(sched),
                            BodyTask<Space>(niters)));
            }

            Kokkos::Future< Space > all = Kokkos::when_all(&futures[0], futures.size());
            Kokkos::respawn(this, all);
        } else if (respawns == 2) {
            const unsigned long long end_time = current_time_ns();
            printf("METRIC flat_parallel_iters %d %.20f\n", PARALLEL_LOOP_RANGE,
                    (double)PARALLEL_LOOP_RANGE / ((double)(end_time -
                            start_time) / 1000.0));
        }
    }

    static void run() {
        typedef typename Kokkos::TaskScheduler< Space >::memory_space memory_space;
        enum { MinBlockSize = 64 };
        enum { MaxBlockSize = 2u * 1024u * 1024u };
        enum { SuperBlockSize = 1u << 12 };
        Kokkos::TaskScheduler< Space > root_sched( memory_space(),
                2u * 1024u * 1024u * 1024u, MinBlockSize, MaxBlockSize, SuperBlockSize);

        Kokkos::Future< void, Space > f = Kokkos::host_spawn(
                Kokkos::TaskSingle( root_sched ), DriverTask( root_sched, omp_get_num_threads()) );

        Kokkos::wait( root_sched );
    }
};

int main(int argc, char **argv) {

    Kokkos::OpenMP::initialize();
    DriverTask<Kokkos::OpenMP>::run();

    return 0;
}
