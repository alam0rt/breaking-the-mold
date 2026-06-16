/* klash_brute_raw: raw-namespace exhaustive preimage brute (id = calcHash(name)).
 *
 *   gcc -O3 -pthread -o klash_brute_raw tools/klash_brute_raw.c
 *   ./klash_brute_raw --min 9 --max 9 --max-overshoot 0 --wordlike <ids-file>
 *
 * Output: "0x<id>\t<string>\tfloor=F\tlen=L"
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

static const char CS[36] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
static int STEP[36];

static int popc(uint32_t v){ int c=0; while(v){v&=v-1;c++;} return c; }
static int floor_of(uint32_t id){ return popc(id); }   /* raw: floor = popc(id) */

static int64_t *slots=NULL; static size_t cap=0;
static void set_init(size_t n){ cap=16; while(cap<n*2) cap<<=1;
    slots=malloc(cap*sizeof(int64_t)); for(size_t i=0;i<cap;i++) slots[i]=-1; }
static void set_add(uint32_t k){ size_t i=(k*2654435761u)&(cap-1);
    while(slots[i]!=-1){ if((uint32_t)slots[i]==k) return; i=(i+1)&(cap-1);} slots[i]=k; }
static inline int set_has(uint32_t k){ size_t i=(k*2654435761u)&(cap-1);
    while(slots[i]!=-1){ if((uint32_t)slots[i]==k) return 1; i=(i+1)&(cap-1);} return 0; }

static pthread_mutex_t outmu = PTHREAD_MUTEX_INITIALIZER;
static int g_overshoot=-1;
static int g_wordlike=0;
static int g_maxcons=3;

static int is_wordlike_dig(const uint8_t *dig, int L){
    int vowels=0,digits=0,run=0,same=1; uint8_t prev=255;
    for(int i=0;i<L;i++){
        char c=CS[dig[i]];
        int isv=(c=='A'||c=='E'||c=='I'||c=='O'||c=='U');
        int isd=(c>='0'&&c<='9');
        if(isd){ digits++; run=0; }
        else if(isv){ vowels++; run=0; }
        else { if(++run>g_maxcons) return 0; }
        if(dig[i]==prev){ if(++same>=3) return 0; } else same=1;
        prev=dig[i];
    }
    if(!vowels) return 0;
    if(digits*2 > L) return 0;
    return 1;
}

typedef struct { int L; uint64_t start, count; } Job;

static void *worker(void *arg){
    Job *j=(Job*)arg; int L=j->L;
    uint8_t dig[16]={0};
    uint64_t idx=j->start;
    for(int p=0;p<L;p++){ dig[p]=idx%36; idx/=36; }
    for(uint64_t n=0;n<j->count;n++){
        uint32_t h=0,sh=0;
        for(int p=0;p<L;p++){ sh=(sh+STEP[dig[p]])&31; h^=1u<<sh; }
        if(set_has(h)){
            int fl=floor_of(h), over=L-fl;
            if(g_overshoot<0 || (over>=0 && over<=g_overshoot)){
                if(g_wordlike && !is_wordlike_dig(dig,L)) goto next;
                char s[17]; for(int p=0;p<L;p++) s[p]=CS[dig[p]]; s[L]=0;
                pthread_mutex_lock(&outmu);
                printf("0x%08x\t%s\tfloor=%d\tlen=%d\n", h, s, fl, L);
                fflush(stdout);
                pthread_mutex_unlock(&outmu);
            }
        }
    next: ;
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
    if(lmax>12){ fprintf(stderr,"refusing --max>12\n"); return 1; }
    if(T<=0){ long n=sysconf(_SC_NPROCESSORS_ONLN); T=(int)(n>0?n:4); }

    for(int d=0;d<36;d++){ char c=CS[d]; int o=c; if(c>='0'&&c<='9') o+=22; STEP[d]=(o-64)&31; }

    FILE *f=fopen(ids,"r"); if(!f){ perror(ids); return 1; }
    char line[256]; size_t cnt=0;
    while(fgets(line,sizeof line,f)) if(line[0]&&line[0]!='\n') cnt++;
    rewind(f); set_init(cnt?cnt:1); size_t added=0;
    while(fgets(line,sizeof line,f)){ char *e; unsigned long v=strtoul(line,&e,0);
        if(e!=line){ set_add((uint32_t)v); added++; } }
    fclose(f);
    fprintf(stderr,"loaded %zu raw ids, %d threads, lengths %d..%d%s%s\n",
        added,T,lmin,lmax,
        g_overshoot>=0?", overshoot-filtered":"",
        g_wordlike?", wordlike-filtered":"");

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
        fprintf(stderr,"  len %2d: %llu strings in %.1fs (%.1fM/s)\n",
            L,(unsigned long long)total,dt,rate/1e6);
    }
    fprintf(stderr,"done.\n");
    return 0;
}
