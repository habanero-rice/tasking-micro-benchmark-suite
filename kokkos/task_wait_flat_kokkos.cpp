#include <Kokkos_Macros.hpp>
#include <Kokkos_Core.hpp>
#include "Kokkos_HostSpace.hpp"
#include "Kokkos_TaskScheduler.hpp"
#include "Kokkos_TaskPolicy.hpp"

#include "timing.h"
#include <stdio.h>
#include "task_wait_flat.h"


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
    int respawns = 0;
    unsigned long long start_time;

    KOKKOS_INLINE_FUNCTION
    DriverTask( const Kokkos::TaskScheduler< Space > & set_sched) : sched(set_sched) {
    }

    KOKKOS_INLINE_FUNCTION
    void operator()( typename Kokkos::TaskScheduler< Space >::member_type &) {
        const int id = respawns++;

        if (id == 0) {
            start_time = current_time_ns();
        }

        if (id < N_FLAT_TASK_WAITS) {
            Kokkos::Future< void, Space > fut = Kokkos::task_spawn(
                    Kokkos::TaskSingle(sched), EmptyTask<Space>());

            Kokkos::respawn(this, fut);
        } else {
            const unsigned long long end_time = current_time_ns();
            printf("METRIC task_wait_flat %d %.20f\n", N_FLAT_TASK_WAITS,
                    (double)N_FLAT_TASK_WAITS / ((double)(end_time -
                            start_time) / 1000.0));
        }
    }

    static void run() {
        typedef typename Kokkos::TaskScheduler< Space >::memory_space memory_space;
        enum { MinBlockSize = 64 };
        enum { MaxBlockSize = 2u * 1024u * 1024u };
        enum { SuperBlockSize = 1u << 12 };
        Kokkos::TaskScheduler< Space > root_sched( memory_space(),
                1024 * 1024 * 1024, MinBlockSize, MaxBlockSize, SuperBlockSize);

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
