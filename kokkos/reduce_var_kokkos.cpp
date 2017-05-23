#include <Kokkos_Macros.hpp>
#include <Kokkos_Core.hpp>
#include "Kokkos_HostSpace.hpp"
#include "Kokkos_TaskScheduler.hpp"
#include "Kokkos_TaskPolicy.hpp"

#include "timing.h"
#include "reduce_var.h"
#include <stdio.h>
#include <assert.h>

template< typename Space >
struct BodyTask
{
    typedef Kokkos::View< long, Space >     accum_type;
    typedef void                            value_type;

    int niters;
    accum_type accum;

    KOKKOS_INLINE_FUNCTION
    BodyTask(int setIters, const accum_type & setAccum) : niters(setIters),
            accum(setAccum) { }

    KOKKOS_INLINE_FUNCTION
    void operator()( typename Kokkos::TaskScheduler< Space >::member_type &) {
        for (int i = 0; i < niters; i++) {
            Kokkos::atomic_increment(&accum());
        }
    }
};

template< typename Space >
struct DriverTask
{
    typedef Kokkos::View< long, Space >     accum_type;
    typedef void                            value_type;

    Kokkos::TaskScheduler< Space >  sched;
    int nchunks;
    accum_type accum;
    std::vector< Kokkos::Future< void, Space > > futures;
    int respawns = 0;
    unsigned long long start_time;

    KOKKOS_INLINE_FUNCTION
    DriverTask( const Kokkos::TaskScheduler< Space > & set_sched,
            int nthreads, const accum_type & setAccum) : sched(set_sched),
            nchunks(2 * nthreads), accum(setAccum) {
    }

    KOKKOS_INLINE_FUNCTION
    void operator()( typename Kokkos::TaskScheduler< Space >::member_type &) {
        respawns++;

        if (respawns == 1) {
            // Measure task creation
            start_time = current_time_ns();

            // Make some reasonable guess at a good chunk size...
            int chunkSize = (NREDUCERS + nchunks - 1) / nchunks;

            for (int launched = 0; launched < nchunks; launched++) {
                int chunkStart = launched * chunkSize;
                int chunkEnd = (launched + 1) * chunkSize;
                if (chunkEnd > NREDUCERS) {
                    chunkEnd = NREDUCERS;
                }
                int niters = chunkEnd - chunkStart;

                futures.push_back(Kokkos::task_spawn(Kokkos::TaskSingle(sched),
                            BodyTask<Space>(niters, accum)));
            }

            Kokkos::Future< Space > all = Kokkos::when_all(&futures[0], futures.size());
            Kokkos::respawn(this, all);
        } else if (respawns == 2) {
            const unsigned long long end_time = current_time_ns();
            printf("METRIC flat_reduction %d %.20f\n", NREDUCERS,
                    (double)NREDUCERS / ((double)(end_time -
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

        Kokkos::View< long, Space > accum("accum");
        typename Kokkos::View< long, Space >::HostMirror host_accum =
            Kokkos::create_mirror_view(accum);

        Kokkos::Future< void, Space > f = Kokkos::host_spawn(
                Kokkos::TaskSingle( root_sched ), DriverTask( root_sched,
                    omp_get_num_threads(), accum) );

        Kokkos::wait( root_sched );

        Kokkos::deep_copy( host_accum, accum );

        assert(host_accum() == NREDUCERS);
    }
};

int main(int argc, char **argv) {

    Kokkos::OpenMP::initialize();
    DriverTask<Kokkos::OpenMP>::run();

    return 0;
}
