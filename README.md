# Habanero Tasking Micro-Benchmark Suite

The Habanero Tasking Micro-Benchmark Suite (HTMB for short) is a collection of
tasking micro-benchmarks implemented on several low-level tasking runtimes.
Today, that includes the HClib, OCR, Realm, TBB, and OpenMP tasking APIs.

These micro-benchmarks are intended to identify relative overheads in expressing
different parallel patterns. As such, most benchmarks consist only of the
creation, scheduling, and execution of some task graph. The goal is to drive
down task creation, scheduling, synchronization, and other costs in these
tasking runtimes and thereby improve their flexibility in terms of the types and
granularities of computational patterns that can be efficiently expressed.

While many tasking runtimes offer their own performance micro-benchmarks, this
suite spans as many tasking runtimes as possible by focusing on high-level
parallel patterns that are expressible across runtimes. Indeed, the patterns
included in this suite are in many cases drawn from other platform-specific
micro-benchmark suites, such as those included with Qthreads, Kokkos, Realm, and
OCR.

## Micro-Benchmarks

Below are the current micro-benchmarks included in this suite, along with short
descriptions of the parallel pattern or overheads they are intended to measure:

1. *bin_fan_out*: Measure time to create a binary tree of tasks up to a set depth.
   There is no intermediate synchronization, we wait on all tasks at once.
2. *fan_out*: Measure time to create, schedule, and execute a set number of tasks
   whose execution are all predicated on the satisfaction of a single event.
3. *fan_out_and_in*: Measure time to do fan_out, followed by collecting back
   into a single event which we then wait on.
4. *future_spawn*: Measure time to create tasks whose execution are predicated
   on events, and time to actually schedule and execute them once those events
   have been satisfied.
5. *parallel_loop*: Measure time to execute a parallel loop with a given number
   of iterations using tasks. The way in which the loop is decomposed into tasks
   is up to the implementor.
6. *prod_cons*: Measure time to pass a set number of messages between a producer
   and a consumer.
7. *reduce_var*: Measure time to collect contributions from a set number of data
   points into a single sum reduction variable.
8. *task_spawn*: Measure time to create, schedule, and execute simple
   non-blocking tasks.
9. *task_wait_flat*: Measure time required to create and immediately wait on a
   set number of tasks.
10. *task_wait_recursive*: Measure time required to create and immediately wait
    on a set number of tasks, where each task in turn creates and waits on a nested task.
11. *unbalanced_bin_fan_out*: The same as bin_fan_out, but with each branch set
    to explore to a different depth.

When running these micro-benchmarks, we generally recommend that they be tested
at varying scales. For example, you might test with a single core, a single
socket, and multiple sockets. Often, runtimes will behave differently as the
memory hierarchy they are running on becomes deeper and the benchmark becomes
more reliant on runtime smarts to perform well.

## Compiling

The provided Makefile will compile all examples, placing the binaries under
bin/. You can compile only the benchmarks for specific runtimes by specifying
the corresponding make target, e.g.:

    $ make ocr

If you are compiling HClib implementations, HCLIB_ROOT is expected to point to
your hclib-install directory. If you are using the resource_workers branch of
HClib, JSMN_HOME must point to your installation of the JSMN library.

If you would like to link in the scalable TBB allocator, set TBB_MALLOC_LIBDIR
to the directory containing libtbbmalloc.so.

If you are compiling the TBB implementations, TBBROOT must be set to the root
TBB installation directory.

If you are compiling the OCR implementations, OCR_INSTALL must be set to the root
OCR installation directory.

If you are compiling the Realm implementations, REALM_LIB must be set to the
path to librealm.a and LG_RT_DIR must be set as required in the Legion install
instructions.

If you are compiling the Qthreads implementations, QTHREADS_HOME must be set to the root Qthread installation directory. Also, if you configured Qthreads to use hwloc, HWLOC_HOME must be set to the root hwloc installation directory. More details on building Qthreads (and with hwloc) can be found in setup_scripts/README.md and setup_scripts/setup_qthreads.sh.

You can change the C and C++ compilers used for these examples by setting the CC
and CXX environment variables.

## Executing

Sample scripts for running all compiled micro-benchmarks are included under
run_scripts/. The provided scripts post-process the output of each benchmark,
producing a CSV file at metrics.csv containing the aggregate results.

For the Qthreads variants, you can use the provided templates under run_scripts and will likely need to experiment with those environment variables to ensure a well-configured Qthreads runtime (Detailed information can be found at: https://goo.gl/qpf29c).

## Contributing

The implementations of the various parallel patterns on each runtime are a
continual work-in-progress. Significant improvements may be possible by those
who are more experienced in developing software on top of these runtimes. We are
always happy to accept pull requests improving a particular implementation of a
micro-benchmark, or adding micro-benchmarks for new runtimes that should be
included in this comparison.
