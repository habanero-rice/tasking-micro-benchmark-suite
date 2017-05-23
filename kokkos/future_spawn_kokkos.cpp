#include <Kokkos_Macros.hpp>
#include <Kokkos_Core.hpp>
#include "Kokkos_HostSpace.hpp"
#include "Kokkos_TaskScheduler.hpp"
#include "Kokkos_TaskPolicy.hpp"

#include "timing.h"
#include <stdio.h>
#include "future_spawn.h"


template< typename Space >
struct EmptyTask
{
    typedef int                            value_type;

    KOKKOS_INLINE_FUNCTION
        void operator()( typename Kokkos::TaskScheduler< Space >::member_type &,
                int & result) {
            int incr = 0;
            incr = incr + 1;
            result = 0;
        }
};

template< typename Space >
struct DriverTask
{
    typedef void                            value_type;

    Kokkos::TaskScheduler< Space >  sched;
    int respawns = 0;
    unsigned long long start_run;

    KOKKOS_INLINE_FUNCTION
    DriverTask( const Kokkos::TaskScheduler< Space > & set_sched) : sched(set_sched) {
    }

    KOKKOS_INLINE_FUNCTION
    void operator()( typename Kokkos::TaskScheduler< Space >::member_type &) {
        respawns++;

        if (respawns == 1) {
            // Measure task creation
            const unsigned long long start_time = current_time_ns();

            Kokkos::Future< void, Space > prev = Kokkos::task_spawn(
                    Kokkos::TaskSingle(sched), EmptyTask<Space>());
            for (int launched = 1; launched < NFUTURES; launched++) {
                prev = Kokkos::task_spawn(Kokkos::TaskSingle(prev),
                        EmptyTask<Space>());
            }

            const unsigned long long end_time = current_time_ns();
            printf("METRIC future_create %d %.20f\n", NFUTURES,
                    (double)NFUTURES / ((double)(end_time - start_time) / 1000.0));

            Kokkos::respawn(this, prev);
        } else if (respawns == 2) {
            start_run = current_time_ns();

            Kokkos::Future< void, Space > prev = Kokkos::task_spawn(
                    Kokkos::TaskSingle(sched), EmptyTask<Space>());
            for (int launched = 1; launched < NFUTURES; launched++) {
                prev = Kokkos::task_spawn(Kokkos::TaskSingle(prev),
                        EmptyTask<Space>());
            }

            Kokkos::respawn(this, prev);
        } else if (respawns == 3) {
            const unsigned long long end_run = current_time_ns();
            printf("METRIC future_run %d %.20f\n", NFUTURES,
                    (double)NFUTURES / ((double)(end_run - start_run) / 1000.0));
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
                Kokkos::TaskSingle( root_sched ), DriverTask( root_sched ) );

        Kokkos::wait( root_sched );
    }
};

int main(int argc, char **argv) {

    Kokkos::OpenMP::initialize();
    DriverTask<Kokkos::OpenMP>::run();

    return 0;
}
