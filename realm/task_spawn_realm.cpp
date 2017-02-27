#include "timing.h"

#include <stdio.h>
#include <stddef.h>
#include "realm/realm.h"
#include "task_spawn.h"

enum {
    TASK_ID = Realm::Processor::TASK_ID_FIRST_AVAILABLE + 0
};

static void task(const void *args, size_t arglen, const void *userdata,
        size_t userlen, Realm::Processor p) {
    arglen = arglen + 1;
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

    int count_cpus = collect_cpus(all_cpus);
    printf("Using %d Realm threads\n", count_cpus);

    {
        std::set<Realm::Event> all_events;
        int nlaunched = 0;
        const unsigned long long spawn_start_time = current_time_ns();
        do {
            Realm::Event e = all_cpus.at(nlaunched % count_cpus).spawn(TASK_ID,
                    NULL, 0);
            all_events.insert(e);
            nlaunched++;
        } while (nlaunched < NTASKS);
        const unsigned long long spawn_end_time = current_time_ns();
        printf("METRIC task_create %d %.20f\n", NTASKS,
                (double)NTASKS / ((double)(spawn_end_time -
                        spawn_start_time) / 1000.0));

        Realm::Event merged = Realm::Event::merge_events(all_events);
        merged.external_wait();
    }

    {
        std::set<Realm::Event> all_events;
        int nlaunched = 0;
        const unsigned long long schedule_start_time = current_time_ns();
        do {
            Realm::Event e = all_cpus.at(nlaunched % count_cpus).spawn(TASK_ID,
                    NULL, 0);
            all_events.insert(e);
            nlaunched++;
        } while (nlaunched < NTASKS);
        Realm::Event merged = Realm::Event::merge_events(all_events);
        merged.external_wait();
        const unsigned long long schedule_end_time = current_time_ns();
        printf("METRIC task_run %d %.20f\n", NTASKS,
                (double)NTASKS / ((double)(schedule_end_time -
                        schedule_start_time) / 1000.0));
    }

    runtime.shutdown();
    runtime.wait_for_shutdown();

    return 0;
}
