/* klash_brute: multithreaded exhaustive preimage brute for Skullmonkeys asset ids.
 *
 * Enumerates every alphanumeric string from length --min up to --max (smallest
 * first), across all CPU cores, and streams matches to stdout as they're found.
 *
 *   gcc -O3 -pthread -o klash_brute tools/klash_brute.c
 *   cut -d, -f2 docs/reference/asset-ids-master.csv | tail -n +2 > /tmp/ids.txt
 *   ./klash_brute --max-overshoot 2 --min 1 --max 9 /tmp/ids.txt | tee found.txt
 *
 * Output (stdout, flushed per hit): "0x<id>\t<string>\tfloor=F\tlen=L"
 * Progress (stderr): per-length rate / ETA.
 *
 * Notes
 *  - Alphabet is A-Z0-9 (36). The hash folds case and ignores non-alnum, so this
 *    is the entire reachable input space; separators add nothing.
 *  - --max-overshoot N keeps only hits with (L - floor) in [0,N]; floor =
 *    popcount(id ^ 0x28C0E011). N=0 = clean names; omit = every collision.
 *  - Cost is 36^L per length: ~feasible to L=8-9 even on big iron, L>=11 is not
 *    (36^11 ~ 1.3e17). Use a wordlist (klash_match) for long names.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

static const char CS[36] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
static int STEP[36];                 /* per-symbol calcHash step */

static uint32_t rotl(uint32_t v,int r){ r&=31; return (v<<r)|(v>>((32-r)&31)); }
static int popc(uint32_t v){ int c=0; while(v){v&=v-1;c++;} return c; }
static int floor_of(uint32_t id){ return popc(id ^ 0x28C0E011u); }

/* shared, read-only after load */
static int64_t *slots=NULL; static size_t cap=0;
static void set_init(size_t n){ cap=16; while(cap<n*2) cap<<=1;
    slots=malloc(cap*sizeof(int64_t)); for(size_t i=0;i<cap;i++) slots[i]=-1; }
static void set_add(uint32_t k){ size_t i=(k*2654435761u)&(cap-1);
    while(slots[i]!=-1){ if((uint32_t)slots[i]==k) return; i=(i+1)&(cap-1);} slots[i]=k; }
static inline int set_has(uint32_t k){ size_t i=(k*2654435761u)&(cap-1);
    while(slots[i]!=-1){ if((uint32_t)slots[i]==k) return 1; i=(i+1)&(cap-1);} return 0; }

static pthread_mutex_t outmu = PTHREAD_MUTEX_INITIALIZER;
static int g_overshoot=-1;
static int g_wordlike=0;     /* drop unpronounceable gibberish */
static int g_maxcons=3;      /* max consecutive consonants allowed */

/* cheap pronounceability heuristic: >=1 vowel, <=50% digits,
   no long consonant run, no XXX same-char run. */
static int is_wordlike(const char *s){
    int len=0,vowels=0,digits=0,run=0,same=1; char prev=0;
    for(const char *p=s;*p;p++){
        char c=*p; len++;
        int isv=(c=='A'||c=='E'||c=='I'||c=='O'||c=='U');
        int isd=(c>='0'&&c<='9');
        if(isd){ digits++; run=0; }
        else if(isv){ vowels++; run=0; }
        else { if(++run>g_maxcons) return 0; }
        if(c==prev){ if(++same>=3) return 0; } else same=1;
        prev=c;
    }
    if(!len || !vowels) return 0;
    if(digits*2 > len) return 0;
    return 1;
}

typedef struct { int L; uint64_t start, count; } Job;

static void *worker(void *arg){
    Job *j=(Job*)arg; int L=j->L;
    uint8_t dig[16]={0};
    uint64_t idx=j->start;
    for(int p=0;p<L;p++){ dig[p]=idx%36; idx/=36; }   /* odometer state at start */
    for(uint64_t n=0;n<j->count;n++){
        /* hash current odometer */
        uint32_t h=0,sh=0;
        for(int p=0;p<L;p++){ sh=(sh+STEP[dig[p]])&31; h^=1u<<sh; }
        uint32_t id=0x28C0E011u ^ rotl(h,27);
        if(set_has(id)){
            int fl=floor_of(id), over=L-fl;
            if(g_overshoot<0 || (over>=0 && over<=g_overshoot)){
                char s[17]; for(int p=0;p<L;p++) s[p]=CS[dig[p]]; s[L]=0;
                if(g_wordlike && !is_wordlike(s)) goto next;
                pthread_mutex_lock(&outmu);
                printf("0x%08x\t%s\tfloor=%d\tlen=%d\n", id, s, fl, L);
                fflush(stdout);
                pthread_mutex_unlock(&outmu);
            }
        }
    next: ;
        /* increment odometer (digit 0 = least significant) */
        int p=0; while(p<L && ++dig[p]==36){ dig[p]=0; p++; }
    }
    return NULL;
}

static uint64_t ipow36(int L){ uint64_t v=1; for(int i=0;i<L;i++) v*=36; return v; }

int main(int argc,char**argv){
    const char *ids=NULL; int lmin=1,lmax=8,T=0;
    for(int i=1;i<argc;i++){
        if(!strcmp(argv[i],"--min")&&i+1<argc) lmin=atoi(argv[++i]);
        else if(!strcmp(argv[i],"--max")&&i+1<argc) lmax=atoi(argv[++i]);
        else if(!strcmp(argv[i],"--threads")&&i+1<argc) T=atoi(argv[++i]);
        else if(!strcmp(argv[i],"--max-overshoot")&&i+1<argc) g_overshoot=atoi(argv[++i]);
        else if(!strcmp(argv[i],"--wordlike")) g_wordlike=1;
        else if(!strcmp(argv[i],"--max-cons")&&i+1<argc) g_maxcons=atoi(argv[++i]);
        else if(!ids) ids=argv[i];
    }
    if(!ids){ fprintf(stderr,"usage: %s [--min M --max N --threads T --max-overshoot K --wordlike --max-cons C] <ids-file>\n",argv[0]); return 1; }
    if(lmax>12){ fprintf(stderr,"refusing --max>12 (36^13 overflows / is infeasible)\n"); return 1; }
    if(T<=0){ long n=sysconf(_SC_NPROCESSORS_ONLN); T=(int)(n>0?n:4); }

    for(int d=0;d<36;d++){ char c=CS[d]; int o=c; if(c>='0'&&c<='9') o+=22; STEP[d]=(o-64)&31; }

    FILE *f=fopen(ids,"r"); if(!f){ perror(ids); return 1; }
    char line[256]; size_t cnt=0;
    while(fgets(line,sizeof line,f)) if(line[0]&&line[0]!='\n') cnt++;
    rewind(f); set_init(cnt?cnt:1); size_t added=0;
    while(fgets(line,sizeof line,f)){ char *e; unsigned long v=strtoul(line,&e,0);
        if(e!=line){ set_add((uint32_t)v); added++; } }
    fclose(f);
    fprintf(stderr,"loaded %zu ids, %d threads, lengths %d..%d%s\n",
        added,T,lmin,lmax, g_overshoot>=0?", overshoot-filtered":"");

    pthread_t *th=malloc(sizeof(pthread_t)*T);
    Job *jobs=malloc(sizeof(Job)*T);
    for(int L=lmin;L<=lmax;L++){
        uint64_t total=ipow36(L), chunk=(total+T-1)/T;
        struct timespec t0; clock_gettime(CLOCK_MONOTONIC,&t0);
        for(int t=0;t<T;t++){
            uint64_t s=(uint64_t)t*chunk;
            jobs[t].L=L; jobs[t].start=s;
            jobs[t].count = s>=total?0 : (s+chunk>total? total-s : chunk);
            pthread_create(&th[t],NULL,worker,&jobs[t]);
        }
        for(int t=0;t<T;t++) pthread_join(th[t],NULL);
        struct timespec t1; clock_gettime(CLOCK_MONOTONIC,&t1);
        double dt=(t1.tv_sec-t0.tv_sec)+(t1.tv_nsec-t0.tv_nsec)/1e9;
        double rate=dt>0?total/dt:0;
        fprintf(stderr,"  len %2d: %llu strings in %.1fs (%.1fM/s) | next len ~%.0fx longer\n",
            L,(unsigned long long)total,dt,rate/1e6,36.0);
    }
    fprintf(stderr,"done.\n");
    return 0;
}
