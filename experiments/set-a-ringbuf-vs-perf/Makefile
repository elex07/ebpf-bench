CLANG ?= clang
LLVM_STRIP ?= llvm-strip
BPFTOOL ?= bpftool
CC ?= gcc

ARCH := $(shell uname -m | sed 's/x86_64/x86/' | sed 's/aarch64/arm64/')
INCLUDES := -I. -Ibpf

BPF_CFLAGS := -g -O2 -target bpf -D__TARGET_ARCH_$(ARCH) $(INCLUDES)
CFLAGS := -g -O2 -Wall $(INCLUDES)
LDFLAGS := -lbpf -lelf -lz -lpthread

.PHONY: all clean workload

all: workload ringbuf_producer.bpf.o perf_producer.bpf.o

workload: workload/syscall_burst
workload/syscall_burst: workload/syscall_burst.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

ringbuf_producer.bpf.o: bpf/ringbuf_producer.bpf.c bpf/common.h
	$(CLANG) $(BPF_CFLAGS) -c $< -o $@
	$(LLVM_STRIP) -g $@

perf_producer.bpf.o: bpf/perf_producer.bpf.c bpf/common.h
	$(CLANG) $(BPF_CFLAGS) -c $< -o $@
	$(LLVM_STRIP) -g $@

%.skel.h: %.bpf.o
	$(BPFTOOL) gen skeleton $< > $@

clean:
	rm -f workload/syscall_burst *.bpf.o *.skel.h
	rm -f user/ringbuf_consumer user/perf_consumer
