/*
 * syscall_burst.c
 *
 * Controlled synthetic workload generator for Experiment Set A.
 *
 * Features:
 * - Deterministic event rates
 * - Multi-core producers (1 / 2 / 4 cores)
 * - Measures producer-side syscall latency (CLOCK_MONOTONIC_RAW)
 *
 * Build:
 *   gcc -O2 -Wall -o syscall_burst syscall_burst.c -lpthread
 *
 * Example:
 *   taskset -c 2 ./syscall_burst -c 1 -r 100000 -d 10 -l
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <pthread.h>
#include <sched.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <getopt.h>

#define MAX_THREADS          16
#define MAX_LATENCY_SAMPLES  2000000
#define WARMUP_EVENTS        5000

struct thread_ctx {
    int          tid;
    int          cpu;
    uint64_t     target_rate;
    double       duration_sec;
    uint64_t     events_done;
    uint64_t     *latency_ns;
    uint64_t     latency_count;
    int          record_latency;
};

static volatile int g_start = 0;
static volatile int g_stop  = 0;

static inline uint64_t nsec_now(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

static inline pid_t do_getpid(void)
{
    return (pid_t)syscall(SYS_getpid);
}

static int compare_u64(const void *a, const void *b)
{
    uint64_t x = *(const uint64_t *)a;
    uint64_t y = *(const uint64_t *)b;
    return (x > y) - (x < y);
}

static void print_percentiles(uint64_t *samples, uint64_t n, const char *label)
{
    if (n < 100) {
        printf("%s: not enough samples (%lu)\n", label, (unsigned long)n);
        return;
    }

    qsort(samples, n, sizeof(uint64_t), compare_u64);

    #define PCT(p) samples[(uint64_t)(((p)/100.0) * (n-1))]

    uint64_t sum = 0;
    for (uint64_t i = 0; i < n; i++)
        sum += samples[i];

    printf("%s samples: %lu\n", label, (unsigned long)n);
    printf("  mean_ns  : %.1f\n", (double)sum / n);
    printf("  min_ns   : %lu\n", (unsigned long)samples[0]);
    printf("  p50_ns   : %lu\n", (unsigned long)PCT(50));
    printf("  p90_ns   : %lu\n", (unsigned long)PCT(90));
    printf("  p99_ns   : %lu\n", (unsigned long)PCT(99));
    printf("  p99.9_ns : %lu\n", (unsigned long)PCT(99.9));
    printf("  p99.99_ns: %lu\n", (unsigned long)PCT(99.99));
    printf("  max_ns   : %lu\n", (unsigned long)samples[n-1]);
}

static void *worker(void *arg)
{
    struct thread_ctx *ctx = arg;
    uint64_t interval_ns = 1000000000ULL / ctx->target_rate;
    uint64_t next_target = 0;
    uint64_t events = 0;

    while (!g_start)
        ;

    uint64_t start = nsec_now();
    next_target = start;

    while (!g_stop) {
        uint64_t now = nsec_now();

        if (now < next_target)
            continue;

        uint64_t t0 = nsec_now();
        (void)do_getpid();
        uint64_t t1 = nsec_now();

        if (ctx->record_latency && events >= WARMUP_EVENTS &&
            ctx->latency_count < MAX_LATENCY_SAMPLES) {
            ctx->latency_ns[ctx->latency_count++] = t1 - t0;
        }

        events++;
        next_target += interval_ns;

        if (next_target + 10 * interval_ns < now)
            next_target = now;
    }

    ctx->events_done = events;
    return NULL;
}

static void usage(const char *prog)
{
    fprintf(stderr,
        "Usage: %s [options]\n"
        "  -c, --cores <n>       Number of producer threads\n"
        "  -r, --rate <n>        Target total events per second\n"
        "  -d, --duration <sec>  Duration in seconds\n"
        "  -l, --latency         Record per-syscall latency\n"
        "  -h, --help\n", prog);
}

int main(int argc, char **argv)
{
    int ncores = 1;
    uint64_t total_rate = 100000;
    double duration = 10.0;
    int record_latency = 0;

    static struct option long_opts[] = {
        {"cores",    required_argument, 0, 'c'},
        {"rate",     required_argument, 0, 'r'},
        {"duration", required_argument, 0, 'd'},
        {"latency",  no_argument,       0, 'l'},
        {"help",     no_argument,       0, 'h'},
        {0, 0, 0, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "c:r:d:lh", long_opts, NULL)) != -1) {
        switch (opt) {
        case 'c': ncores = atoi(optarg); break;
        case 'r': total_rate = strtoull(optarg, NULL, 10); break;
        case 'd': duration = atof(optarg); break;
        case 'l': record_latency = 1; break;
        case 'h':
        default:
            usage(argv[0]);
            return 1;
        }
    }

    if (ncores < 1 || ncores > MAX_THREADS) {
        fprintf(stderr, "Invalid cores: %d\n", ncores);
        return 1;
    }

    printf("=== syscall_burst ===\n");
    printf("cores          : %d\n", ncores);
    printf("total_rate     : %lu events/sec\n", (unsigned long)total_rate);
    printf("per-thread rate: %lu events/sec\n", (unsigned long)(total_rate / ncores));
    printf("duration       : %.1f sec\n", duration);
    printf("record_latency : %s\n\n", record_latency ? "yes" : "no");

    struct thread_ctx ctxs[MAX_THREADS];
    pthread_t threads[MAX_THREADS];
    uint64_t *all_latency = NULL;

    if (record_latency) {
        all_latency = calloc(MAX_LATENCY_SAMPLES, sizeof(uint64_t));
        if (!all_latency) {
            perror("calloc");
            return 1;
        }
    }

    uint64_t per_thread_rate = total_rate / ncores;
    if (per_thread_rate == 0)
        per_thread_rate = 1;

    for (int i = 0; i < ncores; i++) {
        memset(&ctxs[i], 0, sizeof(ctxs[i]));
        ctxs[i].tid = i;
        ctxs[i].target_rate = per_thread_rate;
        ctxs[i].duration_sec = duration;
        ctxs[i].record_latency = record_latency;
        if (record_latency) {
            ctxs[i].latency_ns = all_latency;
            ctxs[i].latency_count = 0;
        }

        if (pthread_create(&threads[i], NULL, worker, &ctxs[i]) != 0) {
            perror("pthread_create");
            return 1;
        }
    }

    g_start = 1;
    usleep((useconds_t)(duration * 1000000.0));
    g_stop = 1;

    uint64_t total_events = 0;
    for (int i = 0; i < ncores; i++) {
        pthread_join(threads[i], NULL);
        total_events += ctxs[i].events_done;
        printf("thread %d: %lu events\n", i, (unsigned long)ctxs[i].events_done);
    }

    printf("\nTotal events : %lu\n", (unsigned long)total_events);
    printf("Actual rate  : %.1f events/sec\n", (double)total_events / duration);

    if (record_latency && ctxs[0].latency_count > 0) {
        printf("\n=== Producer-side syscall latency (getpid) ===\n");
        print_percentiles(all_latency, ctxs[0].latency_count, "getpid");
    }

    free(all_latency);
    return 0;
}
