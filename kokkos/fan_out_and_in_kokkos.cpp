#include <Kokkos_Macros.hpp>
#include <Kokkos_Core.hpp>
#include "Kokkos_HostSpace.hpp"
#include "Kokkos_TaskScheduler.hpp"
#include "Kokkos_TaskPolicy.hpp"

#include "timing.h"
#include <stdio.h>
#include "fan_out_and_in.h"


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
struct PuttingTask
{
    typedef int                            value_type;

    KOKKOS_INLINE_FUNCTION
    void operator()( typename Kokkos::TaskScheduler< Space >::member_type &,
            int & result) {
        result = 42;
    }
};

template< typename Space >
struct DriverTask
{
    typedef void                            value_type;

    Kokkos::TaskScheduler< Space >  sched;
    int respawns = 0;
    std::vector< Kokkos::Future< void, Space > > futures;
    unsigned long long start_time;

    KOKKOS_INLINE_FUNCTION
    DriverTask( const Kokkos::TaskScheduler< Space > & set_sched) : sched(set_sched) {
    }

    KOKKOS_INLINE_FUNCTION
    void operator()( typename Kokkos::TaskScheduler< Space >::member_type &) {
        respawns++;

        if (respawns == 1) {
            // Measure task creation
            start_time = current_time_ns();

            Kokkos::Future< int, Space > trigger = Kokkos::task_spawn(
                    Kokkos::TaskSingle(sched), PuttingTask<Space>());

            for (int launched = 0; launched < FAN_OUT_AND_IN; launched++) {
                futures.push_back(Kokkos::task_spawn(
                            Kokkos::TaskSingle(trigger), EmptyTask<Space>()));
            }

            Kokkos::Future< Space > all = Kokkos::when_all(&futures[0],
                    futures.size());
            Kokkos::respawn(this, all);
        } else if (respawns == 2) {
            unsigned long long end_time = current_time_ns();

            printf("METRIC fan_out_and_in %d %.20f\n", FAN_OUT_AND_IN,
                    (double)FAN_OUT_AND_IN / ((double)(end_time - start_time) /
                        1000.0));
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
