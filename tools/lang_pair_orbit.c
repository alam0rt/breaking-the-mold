/* lang_pair_orbit.c: enumerate all alpha strings of length L1, build hash bucket,
 * then for each pivoted string, check if any rotation of TARGET XOR with hash
 * exists in the bucket. This is the CORRECT search (no individual popcount filter).
 *
 * Build: gcc -O3 -o tools/lang_pair_orbit tools/lang_pair_orbit.c
 * Usage: tools/lang_pair_orbit <target_mask> <length>
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static int STEP_AL[26];
static uint32_t rotl(uint32_t v, int r) { r &= 31; return (v << r) | (v >> ((32 - r) & 31)); }

static inline uint32_t calc_hash(const int *d, int n){
    uint32_t h=0, sh=0;
    for(int i=0;i<n;i++){
        sh = (sh + STEP_AL[d[i]]) & 31;
        h ^= 1u << sh;
    }
    return h;
}

static void to_str(int *d, int n, char *out){
    for(int i=0;i<n;i++) out[i] = 'A' + d[i];
    out[n] = 0;
}

int main(int argc, char **argv){
    uint32_t target = 0x01079563u;
    int len = 5;
    if(argc>1) target = (uint32_t)strtoul(argv[1],NULL,0);
    if(argc>2) len = atoi(argv[2]);
    
    for(int i=0;i<26;i++) STEP_AL[i] = i + 1;  /* A=1..Z=26 */
    
    /* compute 32 rotations of target */
    uint32_t rots[32];
    for(int r=0;r<32;r++) rots[r] = rotl(target, r);
    
    /* count of strings to enumerate */
    size_t N = 1;
    for(int i=0;i<len;i++) N *= 26;
    fprintf(stderr,"target=%#010x len=%d N=%zu\n", target, len, N);
    
    /* compute all hashes, store in array indexed by string idx */
    uint32_t *hashes = malloc(N * sizeof(uint32_t));
    if(!hashes){ perror("malloc"); return 1; }
    
    struct timespec t0; clock_gettime(CLOCK_MONOTONIC,&t0);
    
    int d[16];
    for(size_t idx=0;idx<N;idx++){
        size_t v = idx;
        for(int i=0;i<len;i++){ d[i] = v % 26; v /= 26; }
        hashes[idx] = calc_hash(d, len);
    }
    
    struct timespec t1; clock_gettime(CLOCK_MONOTONIC,&t1);
    double dt1=(t1.tv_sec-t0.tv_sec)+(t1.tv_nsec-t0.tv_nsec)/1e9;
    fprintf(stderr,"hashed in %.1fs\n", dt1);
    
    /* Build open-addressed hash map: hash -> first string idx */
    size_t cap = 1; while(cap < N*2) cap <<= 1;
    uint32_t *hk = calloc(cap, sizeof(uint32_t));  /* hash key */
    uint32_t *hv = calloc(cap, sizeof(uint32_t));  /* string idx + 1 */
    size_t unique = 0;
    
    for(size_t idx=0;idx<N;idx++){
        uint32_t h = hashes[idx];
        size_t i = ((size_t)h * 2654435761u) & (cap-1);
        while(hv[i] != 0){
            if(hk[i] == h) goto skip;
            i = (i+1) & (cap-1);
        }
        hk[i] = h;
        hv[i] = idx + 1;
        unique++;
    skip:;
    }
    
    struct timespec t2; clock_gettime(CLOCK_MONOTONIC,&t2);
    double dt2=(t2.tv_sec-t1.tv_sec)+(t2.tv_nsec-t1.tv_nsec)/1e9;
    fprintf(stderr,"bucketed %zu unique hashes in %.1fs\n", unique, dt2);
    
    /* For each unique entry in hash map, check 32 rotations */
    size_t pairs = 0;
    for(size_t i=0;i<cap;i++){
        if(hv[i] == 0) continue;
        uint32_t h_a = hk[i];
        uint32_t idx_a = hv[i] - 1;
        for(int r=0;r<32;r++){
            uint32_t target_h = h_a ^ rots[r];
            /* lookup */
            size_t j = ((size_t)target_h * 2654435761u) & (cap-1);
            while(hv[j] != 0){
                if(hk[j] == target_h){
                    /* Found pair */
                    uint32_t idx_b = hv[j] - 1;
                    if(idx_a < idx_b){  /* avoid duplicates */
                        char a_str[20], b_str[20];
                        size_t v = idx_a;
                        for(int k=0;k<len;k++){ d[k] = v%26; v/=26; }
                        to_str(d, len, a_str);
                        v = idx_b;
                        for(int k=0;k<len;k++){ d[k] = v%26; v/=26; }
                        to_str(d, len, b_str);
                        printf("%s %s 0x%08x %d\n", a_str, b_str, rots[r], r);
                        pairs++;
                    }
                    break;
                }
                j = (j+1) & (cap-1);
            }
        }
    }
    
    struct timespec t3; clock_gettime(CLOCK_MONOTONIC,&t3);
    double dt3=(t3.tv_sec-t2.tv_sec)+(t3.tv_nsec-t2.tv_nsec)/1e9;
    fprintf(stderr,"found %zu pairs in %.1fs\n", pairs, dt3);
    
    free(hk); free(hv); free(hashes);
    return 0;
}
