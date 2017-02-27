#include "timing.h"

#include "tbb/task_scheduler_init.h"
#include "tbb/flow_graph.h"

#include <assert.h>
#include <stdio.h>
#include "fan_out.h"

int main(int argc, char **argv) {
    int nthreads = tbb::task_scheduler_init::default_num_threads();
    printf("Using %d TBB threads\n", nthreads);

    tbb::flow::graph g;

    const unsigned long long start_time = current_time_ns();
    tbb::flow::broadcast_node<int> *root = new tbb::flow::broadcast_node<int>(g);
    int i;
    for (i = 0; i < FAN_OUT; i++) {
        tbb::flow::function_node<int, int> *new_node =
            new tbb::flow::function_node<int, int>(g, tbb::flow::unlimited,
                    [=](int in) -> int {
                        int incr = in + 1;
                        return incr;
                    });
        new_node->register_predecessor(*root);
    }
    bool success = root->try_put(0);
    assert(success);
    g.wait_for_all();
    const unsigned long long end_time = current_time_ns();
    printf("METRIC fan_out %d %.20f\n", FAN_OUT,
            (double)FAN_OUT / ((double)(end_time - start_time) / 1000.0));

    return 0;
}
