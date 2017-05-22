# Set TBB_MALLOC_LIBDIR to link in the TBB scalable allocator
ifeq ($(TBB_MALLOC_LIBDIR),)
	TBB_FLAGS=
else
	TBB_FLAGS=-L$(TBB_MALLOC_LIBDIR) -ltbbmalloc
endif

HCLIB_TARGETS=bin/task_spawn_hclib bin/task_wait_flat_hclib bin/future_spawn_hclib bin/fan_out_hclib \
	bin/fan_out_and_in_hclib bin/bin_fan_out_hclib bin/parallel_loop_hclib bin/prod_cons_hclib \
	bin/reduce_var_hclib bin/unbalanced_bin_fan_out_hclib
OMP_TARGETS=bin/task_spawn_omp bin/task_wait_flat_omp bin/future_spawn_omp bin/fan_out_omp \
	bin/fan_out_and_in_omp bin/bin_fan_out_omp bin/parallel_loop_omp bin/prod_cons_omp \
	bin/reduce_var_omp bin/unbalanced_bin_fan_out_omp
TBB_TARGETS=bin/task_spawn_tbb bin/task_wait_flat_tbb bin/future_spawn_tbb bin/fan_out_tbb \
	bin/fan_out_and_in_tbb bin/bin_fan_out_tbb bin/parallel_loop_tbb bin/prod_cons_tbb \
	bin/unbalanced_bin_fan_out_tbb
OCR_TARGETS=bin/task_spawn_ocr bin/task_wait_flat_ocr bin/future_spawn_ocr bin/fan_out_ocr \
	bin/fan_out_and_in_ocr bin/bin_fan_out_ocr bin/parallel_loop_ocr bin/prod_cons_ocr \
	bin/reduce_var_ocr bin/unbalanced_bin_fan_out_ocr
REALM_TARGETS=bin/task_spawn_realm bin/task_wait_flat_realm bin/future_spawn_realm bin/fan_out_realm \
	bin/fan_out_and_in_realm bin/bin_fan_out_realm bin/parallel_loop_realm bin/prod_cons_realm \
	bin/unbalanced_bin_fan_out_realm
QTHREADS_TARGETS=bin/task_spawn_qthreads bin/future_spawn_qthreads \
	bin/fan_out_qthreads bin/fan_out_and_in_qthreads bin/bin_fan_out_qthreads \
	bin/parallel_loop_qthreads
KOKKOS_TARGETS=bin/task_spawn_kokkos

CC?=g++
CXX?=g++
FLAGS=-g -O3 -Wall -Werror -Icommon

ifeq ($(JSMN_HOME),)
	HCLIB_LIBS=-lhclib -lhclib_system
else
	HCLIB_LIBS=-lhclib -lhclib_system $(JSMN_HOME)/libjsmn.a
endif

ifeq ($(HWLOC_HOME),)
	QTHREADS_FLAGS=-L$(QTHREADS_HOME)/lib
	QTHREADS_LIBS=-lqthread
else
	QTHREADS_FLAGS=-L$(QTHREADS_HOME)/lib -L$(HWLOC_HOME)/lib
	QTHREADS_LIBS=-lqthread -lhwloc
endif

all: hclib omp tbb ocr realm qthreads kokkos

hclib: $(HCLIB_TARGETS)
omp: $(OMP_TARGETS)
tbb: $(TBB_TARGETS)
ocr: $(OCR_TARGETS)
realm: $(REALM_TARGETS)
qthreads: $(QTHREADS_TARGETS)
kokkos: $(KOKKOS_TARGETS)

bin/%_tbb: tbb/%_tbb.cpp
	$(CC) -std=c++11 $(FLAGS) -o $@ $^ -I$(TBBROOT)/include -ltbb -L$(TBBROOT)/lib/intel64/gcc4.7

bin/%_omp: omp/%_omp.c
	$(CC) -fopenmp $(FLAGS) -o $@ $^ $(TBB_FLAGS)

bin/%_ocr: ocr/%_ocr.c
	$(CXX) $(FLAGS) -o $@ $^ -I$(OCR_INSTALL)/include -locr_x86 -L$(OCR_INSTALL)/lib

bin/%_realm: realm/%_realm.cpp
	$(CXX) $(FLAGS) -o $@ $^ $(REALM_LIB) -I$(LG_RT_DIR) -lrt -ldl

bin/%_hclib: hclib/%_hclib.c
	$(CC) $(FLAGS) -I$(HCLIB_ROOT)/include -L$(HCLIB_ROOT)/lib -L$(HCLIB_ROOT)/../modules/system/lib -o $@ $^ $(TBB_FLAGS) $(HCLIB_LIBS)

bin/%_qthreads: qthreads/%_qthreads.c
	$(CC) $(FLAGS) -I$(QTHREADS_HOME)/include $(QTHREADS_FLAGS) -o $@ $^ $(QTHREADS_LIBS)

bin/%_kokkos: kokkos/%_kokkos.cpp
	$(CXX) $(FLAGS) -std=c++11 -fopenmp -I$(KOKKOS_HOME)/include -o $@ $^ $(KOKKOS_HOME)/lib/libkokkos.a -ldl

clean:
	rm -f -r bin/*
