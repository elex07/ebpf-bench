# Makefile for Experiment Set A – Ring Buffer vs Perf Event Array
# Fixed: removed llvm-strip -g (preserves BTF for CO-RE)

CLANG      ?= clang
BPFTOOL    ?= bpftool
CC         ?= gcc

ARCH := $(shell uname -m | sed 's/x86_64/x86/' | sed 's/aarch64/arm64/')
INCLUDES := -I. -Ibpf -I/usr/include/$(shell uname -m)-linux-gnu

BPF_CFLAGS := -g -O2 -target bpf -D__TARGET_ARCH_$(ARCH) $(INCLUDES)
CFLAGS     := -g -O2 -Wall $(INCLUDES)
LDFLAGS    := -lbpf -lelf -lz -lpthread

.PHONY: all clean workload consumers skeletons

all: workload consumers

# ---------- Workload ----------
workload: workload/syscall_burst
workload/syscall_burst: workload/syscall_burst.c
	$(CC) $(CFLAGS) -o $@ $< -lpthread

# ---------- BPF objects (keep BTF) ----------
bpf/ringbuf_producer.bpf.o: bpf/ringbuf_producer.bpf.c bpf/common.h
	$(CLANG) $(BPF_CFLAGS) -c $< -o $@

bpf/perf_producer.bpf.o: bpf/perf_producer.bpf.c bpf/common.h
	$(CLANG) $(BPF_CFLAGS) -c $< -o $@

# ---------- Skeletons ----------
skeletons: bpf/ringbuf_producer.skel.h bpf/perf_producer.skel.h

bpf/ringbuf_producer.skel.h: bpf/ringbuf_producer.bpf.o
	$(BPFTOOL) gen skeleton $< > $@

bpf/perf_producer.skel.h: bpf/perf_producer.bpf.o
	$(BPFTOOL) gen skeleton $< > $@

# ---------- Userspace consumers ----------
consumers: user/ringbuf_consumer user/perf_consumer

user/ringbuf_consumer: user/ringbuf_consumer.c bpf/ringbuf_producer.skel.h bpf/common.h
	$(CC) $(CFLAGS) -o $@ user/ringbuf_consumer.c $(LDFLAGS)

user/perf_consumer: user/perf_consumer.c bpf/perf_producer.skel.h bpf/common.h
	$(CC) $(CFLAGS) -o $@ user/perf_consumer.c $(LDFLAGS)

clean:
	rm -f workload/syscall_burst
	rm -f bpf/*.bpf.o bpf/*.skel.h
	rm -f user/ringbuf_consumer user/perf_consumer
