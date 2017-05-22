#include <Kokkos_Macros.hpp>
#include <Kokkos_Core.hpp>
#include "Kokkos_HostSpace.hpp"
#include "Kokkos_TaskScheduler.hpp"
#include "Kokkos_TaskPolicy.hpp"

#include "timing.h"
#include <stdio.h>
#include "bin_fan_out.h"

template< typename Space >
struct RecursiveTask
{
    size_t depth;
    Kokkos::TaskScheduler< Space >  sched;

    typedef void                            value_type;

    KOKKOS_INLINE_FUNCTION
    RecursiveTask(size_t setDepth,
            const Kokkos::TaskScheduler< Space > & set_sched) : depth(setDepth),
            sched(set_sched) { }

    KOKKOS_INLINE_FUNCTION
    void operator()( typename Kokkos::TaskScheduler< Space >::member_type &) {
        if (depth < BIN_FAN_OUT_DEPTH) {
            Kokkos::task_spawn(Kokkos::TaskSingle(sched),
                    RecursiveTask<Space>(depth + 1, sched));
            Kokkos::task_spawn(Kokkos::TaskSingle(sched),
                    RecursiveTask<Space>(depth + 1, sched));
        }
    }

    static void run() {
        typedef typename Kokkos::TaskScheduler< Space >::memory_space memory_space;
        enum { Log2_SuperBlockSize = 12 };
        Kokkos::TaskScheduler< Space > root_sched( memory_space(),
                1024 * 1024 * 1024, Log2_SuperBlockSize );

        const unsigned long long start_time = current_time_ns();
        Kokkos::Future< void, Space > f = Kokkos::host_spawn(
                Kokkos::TaskSingle( root_sched ), RecursiveTask( 0, root_sched ) );
        Kokkos::wait( root_sched );
        const unsigned long long end_time = current_time_ns();
        printf("METRIC bin_fan_out %d %.20f\n", BIN_FAN_OUT_DEPTH,
                (double)(1 << BIN_FAN_OUT_DEPTH) /
                ((double)(end_time - start_time) / 1000.0));
    }
};

int main(int argc, char **argv) {

    Kokkos::OpenMP::initialize();
    RecursiveTask<Kokkos::OpenMP>::run();

    return 0;
}
