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
static int H = 0;

__attribute__((always_inline)) static inline void escape(void *p)
{
    __asm__ volatile("" : : "g"(p) : "memory");
}

static inline long long elapsed(struct timespec *t1, struct timespec *t2)
{
    return (long long) (t2->tv_sec - t1->tv_sec) * 1e9 +
           (long long) (t2->tv_nsec - t1->tv_nsec);
}

unsigned rank(reg x, int *y)
{
    // const reg perm = _mm256_set_epi32(7,6,3,2,5,4,1,0);
    reg a = _mm256_load_si256((reg *) y);
    reg b = _mm256_load_si256((reg *) (y + 8));

    reg ca = _mm256_cmpgt_epi32(a, x);
    reg cb = _mm256_cmpgt_epi32(b, x);

    reg c = _mm256_packs_epi32(ca, cb);
    // c = _mm256_permutevar8x32_epi32(c, perm);
    int mask = _mm256_movemask_epi8(c);

    // we need to divide the result by two because we call movemask_epi8 on
    // 16-bit masks:
    return mask ? __builtin_ctz(mask) >> 1 : 16;
}

void permute(int *node)
{
    const reg perm = _mm256_setr_epi32(4, 5, 6, 7, 0, 1, 2, 3);
    reg *middle = (reg *) (node + 4);
    reg x = _mm256_loadu_si256(middle);
    x = _mm256_permutevar8x32_epi32(x, perm);
    _mm256_storeu_si256(middle, x);
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

void build_op(int k)
{
    static int t = 0;
    if (k < nblocks) {
        for (int i = 0; i < B; i++) {
            build_op(go(k, i));
            btree[k][i] = (a < N ? a++ : INT_MAX);
        }
        permute(btree[k]);
        build_op(go(k, B));
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

int height(int n)
{
    // grow the tree until its size exceeds n elements
    int s = 0,  // total size so far
        l = B,  // size of the next layer
        h = 0;  // height so far
    while (s + l - B < n) {
        s += l;
        l *= (B + 1);
        h++;
    }
    return h;
}

const int translate[17] = {0, 1, 2, 3,  8,  9,  10, 11, 4,
                           5, 6, 7, 12, 13, 14, 15, 0};

void update(int *res, int *node, unsigned i)
{
    // int val = node[i];
    int val = node[translate[i]];
    *res = (i < B ? val : *res);
}

int lower_bound_op(int _x)
{
    int k = 0, res = INT_MAX;
    reg x = _mm256_set1_epi32(_x - 1);
    for (int h = 0; h < H; h++) {
        unsigned i = rank(x, btree[k]);
        update(&res, btree[k], i);
        k = go(k, i);
    }
    // the last branch:
    if (k < nblocks) {
        unsigned i = rank(x, btree[k]);
        update(&res, btree[k], i);
    }
    return res;
}

void (*build_exec[3])(int) = {&build, &build, &build_op};

int (*lower_bound_exec[3])(int) = {&lower_bound, &lower_bound, &lower_bound_op};

int main(int argc, char **argv)
{
    int k = 0;
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
    H = height(N);

    if (argc == 3 && argv[2] == "0") {
        k = 0;
        btree = malloc(nblocks * B * sizeof(int));
        if (!btree)
            goto alloc_fail;
    } else {
        k = atoi(argv[2]);
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
    build_exec[k](0);

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
        res = lower_bound_exec[k](target);
        clock_gettime(CLOCK_MONOTONIC, &t2);

        if (res != target) {
            printf("search error, res = %d, target = %d\n", res, target);
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
