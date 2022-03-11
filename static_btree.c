#pragma GCC target("avx2")
#pragma GCC optimize("O3")

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <x86intrin.h>

#define B 16   // block size
#define N 160  // total number of key
#define nblocks ((N + B - 1) / B)

#define P (1 << 21)  // page size in bytes (2MB)
#define T \
    ((64 * nblocks + P - 1) / P * P)  // can only allocate whole number of pages

typedef __m256i reg;

int (*btree)[B];
int a[N] = {0};

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
            btree[k][i] = (t < N ? a[t++] : INT_MAX);
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

int main(int argc, char **argv)
{
    // init sorted array
    for (int i = 0; i < N; i++) {
        a[i] = i;
    }

    if (argc != 2 || argv[1] == "1") {
        btree = malloc(nblocks * B * sizeof(int));
    } else {
        btree = (int(*)[16]) aligned_alloc(P, T);
        madvise(btree, T, MADV_HUGEPAGE);
    }

    // build B-tree by sorted array
    build(0);

    // check B-tree
    for (int i = 0; i < nblocks; i++) {
        for (int j = 0; j < B; j++) {
            printf("%d ", btree[i][j]);
        }
        printf("\n");
    }

    // search target
    printf("%d\n", lower_bound(99));
    return 0;
}
