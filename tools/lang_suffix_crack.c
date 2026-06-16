/* lang_suffix_crack.c: find pairs of length-6 alphabetic strings (a, b) such that
 * calcHash(a) ^ calcHash(b) is a rotation of 0x01079563 (popcount 12).
 *
 * Build:  gcc -O3 -pthread -o tools/lang_suffix_crack tools/lang_suffix_crack.c
 * Usage:  tools/lang_suffix_crack [target_mask=0x01079563] [popcount_filter=12]
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define ALPHA_N 26
static int STEP_AL[ALPHA_N];

static uint32_t rotl(uint32_t v, int r) { r &= 31; return (v << r) | (v >> ((32 - r) & 31)); }
static int popc(uint32_t v) { int c = 0; while (v) { v &= v - 1; c++; } return c; }

/* Compute calcHash for a 6-char alphabetical string given as 6 indices in CS */
static inline uint32_t calc_hash6(int d0,int d1,int d2,int d3,int d4,int d5){
    uint32_t h=0; uint32_t sh=0;
    sh = (sh + STEP_AL[d0]) & 31; h ^= 1u << sh;
    sh = (sh + STEP_AL[d1]) & 31; h ^= 1u << sh;
    sh = (sh + STEP_AL[d2]) & 31; h ^= 1u << sh;
    sh = (sh + STEP_AL[d3]) & 31; h ^= 1u << sh;
    sh = (sh + STEP_AL[d4]) & 31; h ^= 1u << sh;
    sh = (sh + STEP_AL[d5]) & 31; h ^= 1u << sh;
    return h;
}

int main(int argc, char **argv){
    uint32_t target = 0x01079563u;
    int want_popc = 6;
    if(argc>1) target = (uint32_t)strtoul(argv[1],NULL,0);
    if(argc>2) want_popc = atoi(argv[2]);
    
    /* setup step table for A-Z (upper case): step = (c - 64) & 31 */
    for(int i=0;i<ALPHA_N;i++) STEP_AL[i] = ((i + 1)) & 31;  /* A=1..Z=26 */
    
    fprintf(stderr,"target=%#010x want_popc=%d (rotations)\n", target, want_popc);
    
    /* compute all 32 rotations */
    uint32_t rots[32];
    for(int r=0;r<32;r++) rots[r] = rotl(target, r);
    
    /* enumerate all 26^6 = 308M strings, store hashes with popc=want_popc,
     * grouping into a hash map: hash -> list of indices */
    size_t N = 1;
    for(int i=0;i<6;i++) N *= 26;
    fprintf(stderr,"enumerating %zu strings...\n", N);
    
    /* map: bitmask -> first string */
    /* Use open-addressing hash with simple linear probe */
    size_t cap = 1; while(cap < N) cap <<= 1;
    cap *= 2;  /* low load factor for speed */
    uint32_t *hkeys = calloc(cap, sizeof(uint32_t));
    uint32_t *hvals = calloc(cap, sizeof(uint32_t));  /* index */
    
    struct timespec t0; clock_gettime(CLOCK_MONOTONIC,&t0);
    
    size_t kept = 0;
    for(size_t idx=0;idx<N;idx++){
        size_t v = idx;
        int d0 = v % 26; v /= 26;
        int d1 = v % 26; v /= 26;
        int d2 = v % 26; v /= 26;
        int d3 = v % 26; v /= 26;
        int d4 = v % 26; v /= 26;
        int d5 = v;
        uint32_t h = calc_hash6(d0,d1,d2,d3,d4,d5);
        if(popc(h) != want_popc) continue;
        /* insert into hash map */
        size_t i = (h * 2654435761u) & (cap-1);
        while(hvals[i] != 0){
            if(hkeys[i] == h) goto next;  /* already have this hash, skip dups */
            i = (i+1) & (cap-1);
        }
        hkeys[i] = h;
        hvals[i] = (uint32_t)idx + 1;  /* +1 so 0 = empty */
        kept++;
        next:;
    }
    
    struct timespec t1; clock_gettime(CLOCK_MONOTONIC,&t1);
    double dt=(t1.tv_sec-t0.tv_sec)+(t1.tv_nsec-t0.tv_nsec)/1e9;
    fprintf(stderr,"enumerated %zu, kept %zu unique hashes in %.1fs\n", N, kept, dt);
    
    /* Now scan again and emit pairs */
    fprintf(stderr,"finding pairs...\n");
    size_t found = 0;
    for(size_t idx=0;idx<N;idx++){
        size_t v = idx;
        int d0 = v % 26; v /= 26;
        int d1 = v % 26; v /= 26;
        int d2 = v % 26; v /= 26;
        int d3 = v % 26; v /= 26;
        int d4 = v % 26; v /= 26;
        int d5 = v;
        uint32_t h_a = calc_hash6(d0,d1,d2,d3,d4,d5);
        if(popc(h_a) != want_popc) continue;
        for(int r=0;r<32;r++){
            uint32_t h_b = h_a ^ rots[r];
            if(popc(h_b) != want_popc) continue;
            /* lookup h_b */
            size_t i = (h_b * 2654435761u) & (cap-1);
            while(hvals[i] != 0){
                if(hkeys[i] == h_b){
                    /* found a partner. Decode partner index */
                    size_t bidx = hvals[i] - 1;
                    if(bidx <= idx) break;  /* avoid duplicate (a,b)/(b,a) */
                    size_t bv = bidx;
                    int b0=bv%26;bv/=26;int b1=bv%26;bv/=26;int b2=bv%26;bv/=26;
                    int b3=bv%26;bv/=26;int b4=bv%26;bv/=26;int b5=bv;
                    char sa[7]={'A'+d0,'A'+d1,'A'+d2,'A'+d3,'A'+d4,'A'+d5,0};
                    char sb[7]={'A'+b0,'A'+b1,'A'+b2,'A'+b3,'A'+b4,'A'+b5,0};
                    printf("%s\t%s\t%#010x\t%d\n", sa, sb, rots[r], r);
                    found++;
                    break;
                }
                i = (i+1) & (cap-1);
            }
        }
    }
    fprintf(stderr,"found %zu pairs\n", found);
    return 0;
}
