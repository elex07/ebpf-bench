# Makefile for Experiment Set B – XDP vs TC vs fentry
# No llvm-strip -g (preserves BTF)

CLANG   ?= clang
BPFTOOL ?= bpftool
CC      ?= gcc

ARCH := $(shell uname -m | sed 's/x86_64/x86/' | sed 's/aarch64/arm64/')
INCLUDES := -I. -Ibpf -I/usr/include/$(shell uname -m)-linux-gnu

BPF_CFLAGS := -g -O2 -target bpf -D__TARGET_ARCH_$(ARCH) $(INCLUDES)
CFLAGS     := -g -O2 -Wall $(INCLUDES)
LDFLAGS    := -lbpf -lelf -lz

.PHONY: all clean skeletons loaders

all: skeletons loaders

bpf/xdp_prog.bpf.o: bpf/xdp_prog.bpf.c bpf/common.h
	$(CLANG) $(BPF_CFLAGS) -c $< -o $@

bpf/tc_prog.bpf.o: bpf/tc_prog.bpf.c bpf/common.h
	$(CLANG) $(BPF_CFLAGS) -c $< -o $@

bpf/fentry_prog.bpf.o: bpf/fentry_prog.bpf.c bpf/common.h
	$(CLANG) $(BPF_CFLAGS) -c $< -o $@

skeletons: bpf/xdp_prog.skel.h bpf/tc_prog.skel.h bpf/fentry_prog.skel.h

bpf/xdp_prog.skel.h: bpf/xdp_prog.bpf.o
	$(BPFTOOL) gen skeleton $< > $@

bpf/tc_prog.skel.h: bpf/tc_prog.bpf.o
	$(BPFTOOL) gen skeleton $< > $@

bpf/fentry_prog.skel.h: bpf/fentry_prog.bpf.o
	$(BPFTOOL) gen skeleton $< > $@

loaders: user/xdp_loader user/tc_loader user/fentry_loader

user/xdp_loader: user/xdp_loader.c bpf/xdp_prog.skel.h bpf/common.h
	$(CC) $(CFLAGS) -o $@ user/xdp_loader.c $(LDFLAGS)

user/tc_loader: user/tc_loader.c bpf/tc_prog.skel.h bpf/common.h
	$(CC) $(CFLAGS) -o $@ user/tc_loader.c $(LDFLAGS)

user/fentry_loader: user/fentry_loader.c bpf/fentry_prog.skel.h bpf/common.h
	$(CC) $(CFLAGS) -o $@ user/fentry_loader.c $(LDFLAGS)

clean:
	rm -f bpf/*.bpf.o bpf/*.skel.h
	rm -f user/xdp_loader user/tc_loader user/fentry_loader
