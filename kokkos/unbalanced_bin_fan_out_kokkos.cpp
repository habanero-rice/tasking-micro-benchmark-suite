#include <Kokkos_Macros.hpp>
#include <Kokkos_Core.hpp>
#include "Kokkos_HostSpace.hpp"
#include "Kokkos_TaskScheduler.hpp"
#include "Kokkos_TaskPolicy.hpp"

#include "timing.h"
#include <stdio.h>
#include "unbalanced_bin_fan_out.h"

template< typename Space >
struct RecursiveTask
{
    size_t depth;
    size_t branch;
    Kokkos::TaskScheduler< Space >  sched;

    typedef void                            value_type;

    KOKKOS_INLINE_FUNCTION
    RecursiveTask(size_t setDepth, size_t setBranch,
            const Kokkos::TaskScheduler< Space > & set_sched) : depth(setDepth),
            branch(setBranch), sched(set_sched) { }

    KOKKOS_INLINE_FUNCTION
    void operator()( typename Kokkos::TaskScheduler< Space >::member_type &) {
        size_t depth_limit = branch * BIN_FAN_OUT_DEPTH_MULTIPLIER;

        if (depth < depth_limit) {
            Kokkos::task_spawn(Kokkos::TaskSingle(sched),
                    RecursiveTask<Space>(depth + 1, branch, sched));
            Kokkos::task_spawn(Kokkos::TaskSingle(sched),
                    RecursiveTask<Space>(depth + 1, branch, sched));
        }
    }

    static void run() {
        typedef typename Kokkos::TaskScheduler< Space >::memory_space memory_space;
        enum { MinBlockSize = 64 };
        enum { MaxBlockSize = 2u * 1024u * 1024u };
        enum { SuperBlockSize = 1u << 12 };
        Kokkos::TaskScheduler< Space > root_sched( memory_space(),
                1024 * 1024 * 1024, MinBlockSize, MaxBlockSize, SuperBlockSize);

        unsigned ntasks = 0;
        const unsigned long long start_time = current_time_ns();
        for (int i = 0; i < N_BRANCHES; i++) {
            ntasks += (1 << (i * BIN_FAN_OUT_DEPTH_MULTIPLIER));
            Kokkos::host_spawn(Kokkos::TaskSingle( root_sched ),
                    RecursiveTask( 0, i, root_sched ) );
        }
        Kokkos::wait( root_sched );
        const unsigned long long end_time = current_time_ns();
        printf("METRIC unbalanced_bin_fan_out %d|%d %.20f\n", N_BRANCHES,
                BIN_FAN_OUT_DEPTH_MULTIPLIER,
                (double)ntasks / ((double)(end_time - start_time) / 1000.0));
    }
};

int main(int argc, char **argv) {

    Kokkos::OpenMP::initialize();
    RecursiveTask<Kokkos::OpenMP>::run();

    return 0;
}
