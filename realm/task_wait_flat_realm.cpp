#include "timing.h"

#include "realm/realm.h"
#include <stdio.h>
#include "task_wait_flat.h"

static Realm::Processor aggregateCpu;

enum {
    TASK_ID = Realm::Processor::TASK_ID_FIRST_AVAILABLE + 0,
    DRIVER_ID
};

static void task(const void *args, size_t arglen, const void *userdata,
        size_t userlen, Realm::Processor p) {
    arglen = arglen + 1;
}

static void driver_task(const void *args, size_t arglen, const void *userdata,
        size_t userlen, Realm::Processor p) {
    const unsigned long long start_time = current_time_ns();
    int i;
    for (i = 0; i < N_FLAT_TASK_WAITS; i++) {
        Realm::Event e = aggregateCpu.spawn(TASK_ID, NULL, 0);
        e.wait();
    }
    const unsigned long long end_time = current_time_ns();
    printf("METRIC task_wait_flat %d %.20f\n", N_FLAT_TASK_WAITS,
            (double)N_FLAT_TASK_WAITS / ((double)(end_time -
                    start_time) / 1000.0));
}

static int collect_cpus(std::vector<Realm::Processor> &all_cpus) {
    std::set<Realm::Processor> all_procs;
    Realm::Machine::get_machine().get_all_processors(all_procs);

    int count_cpus = 0;
    for (std::set<Realm::Processor>::iterator i = all_procs.begin(),
            e = all_procs.end(); i != e; i++) {
        Realm::Processor curr = *i;
        if (curr.kind() == Realm::Processor::LOC_PROC) {
            all_cpus.push_back(curr);
            count_cpus++;
        }
    }

    return count_cpus;
}

int main(int argc, char **argv) {
    Realm::Runtime runtime;
    std::vector<Realm::Processor> all_cpus;
    runtime.init(&argc, &argv);
    runtime.register_task(TASK_ID, task);
    runtime.register_task(DRIVER_ID, driver_task);

    int count_cpus = collect_cpus(all_cpus);
    aggregateCpu = Realm::Processor::create_group(all_cpus);
    printf("Using %d Realm threads\n", count_cpus);

    Realm::Event e = aggregateCpu.spawn(DRIVER_ID, NULL, 0);
    e.external_wait();

    runtime.shutdown();
    runtime.wait_for_shutdown();

    return 0;
}
