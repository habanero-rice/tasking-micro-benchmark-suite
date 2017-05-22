#include <Kokkos_Macros.hpp>
#include <Kokkos_Core.hpp>
#include "Kokkos_HostSpace.hpp"
#include "Kokkos_TaskScheduler.hpp"
#include "Kokkos_TaskPolicy.hpp"

#include "timing.h"
#include <stdio.h>
#include "task_spawn.h"


template< typename Space >
struct EmptyTask
{
    typedef void                            value_type;

    KOKKOS_INLINE_FUNCTION
        void operator()( typename Kokkos::TaskScheduler< Space >::member_type &) {
            int incr = 0;
            incr = incr + 1;
        }
};

template< typename Space >
struct DriverTask
{
    typedef void                            value_type;

    Kokkos::TaskScheduler< Space >  sched;
    std::vector< Kokkos::Future< void, Space > > futures;
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

            for (int launched = 0; launched < NTASKS; launched++) {
                futures.push_back(Kokkos::task_spawn(Kokkos::TaskSingle(sched),
                            EmptyTask<Space>()));
            }

            const unsigned long long end_time = current_time_ns();
            printf("METRIC task_create %d %.20f\n", NTASKS,
                    (double)NTASKS / ((double)(end_time - start_time) / 1000.0));

            Kokkos::Future< Space > all = Kokkos::when_all(&futures[0], futures.size());
            Kokkos::respawn(this, all);
        } else if (respawns == 2) {
            futures.clear();

            start_run = current_time_ns();

            for (int launched = 0; launched < NTASKS; launched++) {
                futures.push_back(Kokkos::task_spawn(Kokkos::TaskSingle(sched),
                            EmptyTask<Space>()));
            }

            Kokkos::Future< Space > all = Kokkos::when_all(&futures[0], futures.size());
            Kokkos::respawn(this, all);
        } else if (respawns == 3) {
            const unsigned long long end_run = current_time_ns();
            printf("METRIC task_run %d %.20f\n", NTASKS,
                    (double)NTASKS / ((double)(end_run - start_run) / 1000.0));
        }
    }

    static void run() {
        typedef typename Kokkos::TaskScheduler< Space >::memory_space memory_space;
        enum { Log2_SuperBlockSize = 12 };
        Kokkos::TaskScheduler< Space > root_sched( memory_space(),
                1024 * 1024 * 1024, Log2_SuperBlockSize );

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
