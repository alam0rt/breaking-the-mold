/* klash_prefix_join: find a short PREFIX that completes known item SUFFIXes to
 * their sprite ids. name = PREFIX + SUFFIX, calcHash(name) == target (RAW) or
 * WRAP(=0x28C0E011 ^ rotl(calcHash,27)) == target. A prefix that hits >=2 pickups
 * is the (corroborated) sprite namespace.
 *
 *   gcc -O3 -pthread -o klash_prefix_join tools/klash_prefix_join.c
 *   ./klash_prefix_join --min 0 --max 6 \
 *       --pair 0xa9240484:ONE_UP --pair 0x80e85ea0:1970 --pair 0x902c0002:SUPER_WILLIE
 *
 * Output: prefixes with >=1 hit, "PREFIX  hits=N  [RAW 0x..=PREFIX+SUF ...]"
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

static const char CS[36] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
static int STEP[36];
#define SEED 0x28C0E011u

static inline uint32_t rotl(uint32_t v,int r){ r&=31; return (v<<r)|(v>>((32-r)&31)); }

/* calcHash of an arbitrary C string (skips non-alnum, like the engine) */
static void chs(const char*s, uint32_t*ph, int*psh){
    uint32_t h=0; int sh=0;
    for(;*s;s++){ int c=(unsigned char)*s, o;
        if(c>='a'&&c<='z') c-=32;
        if(c>='A'&&c<='Z') o=c;
        else if(c>='0'&&c<='9') o=c+22;
        else continue;
        sh=(sh+((o-64)&31))&31; h^=1u<<sh; }
    *ph=h; *psh=sh;
}

#define MAXP 4096
static int NP=0;
static uint32_t g_target[MAXP];
static uint32_t g_sufh[MAXP];
static int g_sufsh[MAXP];
static char g_sufstr[MAXP][40];
static int g_tidx[MAXP];          /* distinct-target index for this pair */
static uint32_t g_distinct[64]; static int g_ndist=0;
static int g_wrap=1;
static int g_mindistinct=2;       /* only report prefixes hitting >= this many DISTINCT targets */

static int target_index(uint32_t t){
    for(int i=0;i<g_ndist;i++) if(g_distinct[i]==t) return i;
    g_distinct[g_ndist]=t; return g_ndist++;
}

static pthread_mutex_t outmu=PTHREAD_MUTEX_INITIALIZER;

typedef struct{int L; uint64_t start,count;} Job;

static void *worker(void*arg){
    Job*j=(Job*)arg; int L=j->L; uint8_t dig[16]={0};
    uint64_t idx=j->start; for(int p=0;p<L;p++){dig[p]=idx%36; idx/=36;}
    for(uint64_t n=0;n<j->count;n++){
        uint32_t hP=0; int shP=0;
        for(int p=0;p<L;p++){ shP=(shP+STEP[dig[p]])&31; hP^=1u<<shP; }
        /* test against each pair, tracking DISTINCT targets hit */
        uint64_t mask=0; char buf[2048]; int bo=0;
        for(int k=0;k<NP;k++){
            uint32_t raw = hP ^ rotl(g_sufh[k], shP);   /* calcHash(PREFIX+SUFFIX) */
            int hit=0; const char*mdl=0;
            if(raw==g_target[k]){ hit=1; mdl="RAW"; }
            else if(g_wrap && (SEED ^ rotl(raw,27))==g_target[k]){ hit=1; mdl="WRAP"; }
            if(hit){ mask|=(1ull<<g_tidx[k]);
                char pre[17]; for(int p=0;p<L;p++) pre[p]=CS[dig[p]]; pre[L]=0;
                if(bo<1900) bo+=snprintf(buf+bo,sizeof(buf)-bo," [%s 0x%08x=%s_%s]",mdl,g_target[k],pre,g_sufstr[k]);
            }
        }
        int nd=__builtin_popcountll(mask);
        if(nd>=g_mindistinct){
            char pre[17]; for(int p=0;p<L;p++) pre[p]=CS[dig[p]]; pre[L]=0;
            pthread_mutex_lock(&outmu);
            printf("%-8s distinct=%d%s\n", L?pre:"(empty)", nd, buf);
            fflush(stdout);
            pthread_mutex_unlock(&outmu);
        }
        int p=0; while(p<L && ++dig[p]==36){ dig[p]=0; p++; }
    }
    return NULL;
}

static uint64_t ipow36(int L){ uint64_t v=1; for(int i=0;i<L;i++) v*=36; return v; }

int main(int argc,char**argv){
    int lmin=0,lmax=6,T=0;
    for(int i=1;i<argc;i++){
        if(!strcmp(argv[i],"--min")&&i+1<argc) lmin=atoi(argv[++i]);
        else if(!strcmp(argv[i],"--max")&&i+1<argc) lmax=atoi(argv[++i]);
        else if(!strcmp(argv[i],"--threads")&&i+1<argc) T=atoi(argv[++i]);
        else if(!strcmp(argv[i],"--no-wrap")) g_wrap=0;
        else if(!strcmp(argv[i],"--min-distinct")&&i+1<argc) g_mindistinct=atoi(argv[++i]);
        else if(!strcmp(argv[i],"--pairfile")&&i+1<argc){
            FILE*f=fopen(argv[++i],"r"); if(!f){perror("pairfile");return 1;}
            char line[128];
            while(fgets(line,sizeof line,f)){
                char*nl=strchr(line,'\n'); if(nl)*nl=0;
                char*c=strchr(line,':'); if(!c||!line[0]) continue;
                *c=0; g_target[NP]=strtoul(line,NULL,0);
                strncpy(g_sufstr[NP],c+1,39); chs(c+1,&g_sufh[NP],&g_sufsh[NP]);
                g_tidx[NP]=target_index(g_target[NP]); NP++;
                if(NP>=MAXP){fprintf(stderr,"too many pairs\n");break;}
            }
            fclose(f);
        }
        else if(!strcmp(argv[i],"--pair")&&i+1<argc){
            char*a=argv[++i]; char*c=strchr(a,':'); if(!c){fprintf(stderr,"bad pair %s\n",a);return 1;}
            *c=0; g_target[NP]=strtoul(a,NULL,0);
            strncpy(g_sufstr[NP],c+1,39); chs(c+1,&g_sufh[NP],&g_sufsh[NP]);
            g_tidx[NP]=target_index(g_target[NP]); NP++;
        }
    }
    if(NP==0){ fprintf(stderr,"need --pair 0xID:SUFFIX ...\n"); return 1; }
    if(lmax>7){ fprintf(stderr,"refusing --max>7 (too slow)\n"); return 1; }
    if(T<=0){ long n=sysconf(_SC_NPROCESSORS_ONLN); T=(int)(n>0?n:4); }
    for(int d=0;d<36;d++){ char c=CS[d]; int o=c; if(c>='0'&&c<='9') o+=22; STEP[d]=(o-64)&31; }
    fprintf(stderr,"%d pairs (%d distinct targets), prefixes len %d..%d, %d threads, wrap=%d, min-distinct=%d\n",
            NP,g_ndist,lmin,lmax,T,g_wrap,g_mindistinct);

    pthread_t*th=malloc(sizeof(pthread_t)*T); Job*jobs=malloc(sizeof(Job)*T);
    for(int L=lmin;L<=lmax;L++){
        if(L==0){ /* empty prefix */
            Job j={0,0,1}; worker(&j); continue;
        }
        uint64_t total=ipow36(L), chunk=(total+T-1)/T;
        for(int t=0;t<T;t++){ uint64_t s=(uint64_t)t*chunk; jobs[t].L=L; jobs[t].start=s;
            jobs[t].count=s>=total?0:(s+chunk>total?total-s:chunk);
            pthread_create(&th[t],NULL,worker,&jobs[t]); }
        for(int t=0;t<T;t++) pthread_join(th[t],NULL);
        fprintf(stderr,"  len %d done (%llu)\n",L,(unsigned long long)total);
    }
    return 0;
}
