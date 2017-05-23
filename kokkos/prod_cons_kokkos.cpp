#include <Kokkos_Macros.hpp>
#include <Kokkos_Core.hpp>
#include "Kokkos_HostSpace.hpp"
#include "Kokkos_TaskScheduler.hpp"
#include "Kokkos_TaskPolicy.hpp"

#include "timing.h"
#include <stdio.h>
#include "prod_cons.h"


template< typename Space >
struct ProducerTask
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
struct ConsumerTask
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
    std::vector< Kokkos::Future< int, Space > > prodFutures;
    std::vector< Kokkos::Future< void, Space > > consFutures;
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

            for (int i = 0; i < PROD_CONS_MSGS; i++) {
                prodFutures.push_back(Kokkos::task_spawn(
                            Kokkos::TaskSingle(sched), ProducerTask<Space>()));
            }

            for (int i = 0; i < PROD_CONS_MSGS; i++) {
                consFutures.push_back(Kokkos::task_spawn(
                            Kokkos::TaskSingle(sched), ConsumerTask<Space>()));
            }

            Kokkos::Future< Space > all = Kokkos::when_all(&consFutures[0],
                    consFutures.size());
            Kokkos::respawn(this, all);
        } else if (respawns == 2) {
            unsigned long long end_time = current_time_ns();

            printf("METRIC producer_consumer %d %.20f\n", PROD_CONS_MSGS,
                    (double)PROD_CONS_MSGS / ((double)(end_time - start_time) /
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
