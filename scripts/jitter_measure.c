/*
 * jitter_measure.c
 * High-resolution timer jitter measurement using CLOCK_MONOTONIC_RAW.
 * Compiles with: gcc -O2 -o jitter_measure jitter_measure.c
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#define MAX_SAMPLES 500000
#define WARMUP 5000

static inline long long diff_ns(const struct timespec *start, const struct timespec *end) {
    return (end->tv_sec - start->tv_sec) * 1000000000LL + (end->tv_nsec - start->tv_nsec);
}

int compare_ll(const void *a, const void *b) {
    long long x = *(const long long *)a;
    long long y = *(const long long *)b;
    return (x > y) - (x < y);
}

int main(int argc, char *argv[]) {
    int duration_sec = 20;
    if (argc > 1) {
        duration_sec = atoi(argv[1]);
        if (duration_sec < 5) duration_sec = 5;
        if (duration_sec > 120) duration_sec = 120;
    }

    long long *samples = malloc(sizeof(long long) * MAX_SAMPLES);
    if (!samples) {
        perror("malloc");
        return 1;
    }

    struct timespec start_wall, now, prev;
    clock_gettime(CLOCK_MONOTONIC_RAW, &start_wall);
    clock_gettime(CLOCK_MONOTONIC_RAW, &prev);

    long long end_ns = (long long)duration_sec * 1000000000LL;
    int count = 0;

    while (count < MAX_SAMPLES) {
        clock_gettime(CLOCK_MONOTONIC_RAW, &now);
        long long elapsed = diff_ns(&start_wall, &now);
        if (elapsed >= end_ns) break;

        long long delta = diff_ns(&prev, &now);
        if (count >= WARMUP) {
            samples[count - WARMUP] = delta;
        }
        count++;
        prev = now;
    }

    int n = count - WARMUP;
    if (n < 1000) {
        fprintf(stderr, "Too few samples collected: %d\n", n);
        free(samples);
        return 1;
    }

    qsort(samples, n, sizeof(long long), compare_ll);

    long long sum = 0;
    for (int i = 0; i < n; i++) sum += samples[i];
    double mean = (double)sum / n;

    /* Percentile helper */
    #define PCT(p) samples[(int)(((p)/100.0) * (n-1))]

    printf("samples_collected: %d\n", n);
    printf("duration_sec: %d\n", duration_sec);
    printf("mean_ns: %.1f\n", mean);
    printf("min_ns: %lld\n", samples[0]);
    printf("p50_ns: %lld\n", PCT(50));
    printf("p90_ns: %lld\n", PCT(90));
    printf("p99_ns: %lld\n", PCT(99));
    printf("p99.9_ns: %lld\n", PCT(99.9));
    printf("p99.99_ns: %lld\n", PCT(99.99));
    printf("max_ns: %lld\n", samples[n-1]);

    free(samples);
    return 0;
}
