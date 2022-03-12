#pragma GCC target("avx2")
#pragma GCC optimize("O3")

#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <time.h>
#include <x86intrin.h>

#define B 16  // block size
// #define N 160  // total number of key
// #define nblocks ((N + B - 1) / B)

#define P (1 << 21)  // page size in bytes (2MB)
// #define T \
//     ((64 * nblocks + P - 1) / P * P)  // can only allocate whole number of pages

typedef __m256i reg;

int (*btree)[B];
static int a = 0;
static int N = 160;
static int nblocks = 0;
static int T = 0;

static inline long long elapsed(struct timespec *t1, struct timespec *t2)
{
    return (long long) (t2->tv_sec - t1->tv_sec) * 1e9 +
           (long long) (t2->tv_nsec - t1->tv_nsec);
}

int go(int k, int i)
{
    return k * (B + 1) + i + 1;
}

void build(int k)
{
    static int t = 0;
    if (k < nblocks) {
        for (int i = 0; i < B; i++) {
            build(go(k, i));
            btree[k][i] = (a < N ? a++ : INT_MAX);
        }
        build(go(k, B));
    }
}

int cmp(reg x_vec, int *y_ptr)
{
    reg y_vec = _mm256_load_si256((reg *) y_ptr);  // load 8 sorted elements
    reg mask = _mm256_cmpgt_epi32(x_vec, y_vec);   // compare against the key
    return _mm256_movemask_ps((__m256) mask);      // extract the 8-bit mask
}

int lower_bound(int _x)
{
    int k = 0, res = INT_MAX;
    reg x = _mm256_set1_epi32(_x);  // copy 8 int _x to x
    while (k < nblocks) {
        int mask = ~(cmp(x, &btree[k][0]) + (cmp(x, &btree[k][8]) << 8));
        int i = __builtin_ffs(mask) - 1;
        if (i < B)
            res = btree[k][i];
        k = go(k, i);
    }
    return res;
}

__attribute__((always_inline)) static inline void escape(void *p)
{
    __asm__ volatile("" : : "g"(p) : "memory");
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        printf("Error: static_btree needs at least 2 arguments\n");
        printf("./static_btree <number> [method]\n");
        return -1;
    }

    struct timespec t1, t2;

    N = 1 << atoi(argv[1]);

    nblocks = (N + B - 1) / B;
    T = ((64 * nblocks + P - 1) / P *
         P);  // can only allocate whole number of pages

    if (argc == 3 && argv[2] == "0") {
        btree = malloc(nblocks * B * sizeof(int));
        if (!btree)
            goto alloc_fail;
    } else {
        btree = (int(*)[B]) aligned_alloc(P, T);
        if (!btree)
            goto alloc_fail;
        if (madvise(btree, T, MADV_HUGEPAGE)) {
            printf("huge page setting failed\n");
            free(btree);
            return -1;
        }
    }

    // build B-tree by sorted array
    build(0);

    // check B-tree
    // for (int i = 0; i < nblocks; i++) {
    //     for (int j = 0; j < B; j++) {
    //         printf("%d ", btree[i][j]);
    //     }
    //     printf("\n");
    // }

    // search target
    int target, res;
    int n = 1024;
    // int n = N * 2;
    long long el;
    double avg = 0;
    srand(0);
    for (int i = 0; i < n; i++) {
        target = rand() % N;
        clock_gettime(CLOCK_MONOTONIC, &t1);
        res = lower_bound(target);
        clock_gettime(CLOCK_MONOTONIC, &t2);

        if (res != target) {
            printf("search error\n");
            return -2;
        }
        // moving average
        el = elapsed(&t1, &t2);
        double delta_intvl = (double) el - avg;
        avg += delta_intvl / (i + 1);
    }
    printf("%f\n", avg);

    free(btree);
    return 0;

alloc_fail:
    printf("allocate memory %d failed\n", N);
    return -1;
}
