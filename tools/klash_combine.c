/* klash_combine: word-combinator preimage search for Skullmonkeys asset ids.
 *
 * Instead of brute-forcing characters (36^len, hopeless past len 9), this
 * enumerates every combination of up to D words from a wordlist -- e.g.
 * MONKEY, MONKEY+BALL, MONKEY+BALL+POP -- which is the "reduced version of
 * every combination". Separators (_,-,space) are ignored by the hash, so word
 * joins are free; only the word sequence matters.
 *
 * Combos are hashed incrementally:  calcHash(a+b) = bits(a) ^ rotl(bits(b), ss(a))
 * where bits(w)=calcHash(w) and ss(w)=sum of char-steps mod 32. So each combo
 * costs a couple of ops, and the inner loop never touches characters.
 *
 *   gcc -O3 -pthread -o klash_combine tools/klash_combine.c
 *   ./klash_combine --depth 3 --max-overshoot 2 --wordlike words.txt ids.txt
 *
 * Output (stdout, flushed): "0x<id>\t<word_word_word>\tfloor=F\tlen=L"
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

static uint32_t rotl(uint32_t v,int r){ r&=31; return (v<<r)|(v>>((32-r)&31)); }
static int popc(uint32_t v){ int c=0; while(v){v&=v-1;c++;} return c; }
static int floor_of(uint32_t id){ return popc(id ^ 0x28C0E011u); }

/* words */
static char  **W=NULL; static uint32_t *WB=NULL; static int *WS=NULL, *WL=NULL; static int NW=0;
/* target set */
static int64_t *slots=NULL; static size_t cap=0;
static void set_init(size_t n){ cap=16; while(cap<n*2) cap<<=1;
    slots=malloc(cap*sizeof(int64_t)); for(size_t i=0;i<cap;i++) slots[i]=-1; }
static void set_add(uint32_t k){ size_t i=(k*2654435761u)&(cap-1);
    while(slots[i]!=-1){ if((uint32_t)slots[i]==k) return; i=(i+1)&(cap-1);} slots[i]=k; }
static inline int set_has(uint32_t k){ size_t i=(k*2654435761u)&(cap-1);
    while(slots[i]!=-1){ if((uint32_t)slots[i]==k) return 1; i=(i+1)&(cap-1);} return 0; }

static int g_depth=3, g_overshoot=-1, g_wordlike=0, g_maxcons=3;
static pthread_mutex_t outmu=PTHREAD_MUTEX_INITIALIZER;

static int is_wordlike(const char *s){
    int len=0,vow=0,dig=0,run=0,same=1,prev=0;
    for(; *s; s++){ int c=*s; if(c=='_'||c=='-') { run=0; same=1; prev=0; continue; }
        len++; int isv=(c=='A'||c=='E'||c=='I'||c=='O'||c=='U'); int isd=(c>='0'&&c<='9');
        if(isd){dig++;run=0;} else if(isv){vow++;run=0;} else { if(++run>g_maxcons) return 0; }
        if(c==prev){ if(++same>=3) return 0; } else same=1; prev=c; }
    return len&&vow&&dig*2<=len;
}

static void emit(int *idx,int depth,uint32_t id){
    char buf[256]; int p=0, alnum=0;
    for(int d=0; d<depth; d++){ if(d) buf[p++]='_';
        memcpy(buf+p, W[idx[d]], WL[idx[d]]); p+=WL[idx[d]]; alnum+=WL[idx[d]]; }
    buf[p]=0;
    int fl=floor_of(id), over=alnum-fl;
    if(g_overshoot>=0 && (over<0||over>g_overshoot)) return;
    if(g_wordlike && !is_wordlike(buf)) return;
    pthread_mutex_lock(&outmu);
    printf("0x%08x\t%s\tfloor=%d\tlen=%d\n", id, buf, fl, alnum); fflush(stdout);
    pthread_mutex_unlock(&outmu);
}

static void rec(int depth, uint32_t h, uint32_t sh, int *idx){
    uint32_t id=0x28C0E011u ^ rotl(h,27);
    if(set_has(id)) emit(idx,depth,id);
    if(depth>=g_depth) return;
    for(int w=0; w<NW; w++){ idx[depth]=w;
        rec(depth+1, h ^ rotl(WB[w],sh), (sh+WS[w])&31, idx); }
}

typedef struct { int lo,hi; } Slice;
static void *worker(void *a){
    Slice *s=(Slice*)a; int idx[16];
    for(int w=s->lo; w<s->hi; w++){ idx[0]=w; rec(1, WB[w], WS[w], idx); }
    return NULL;
}

static void load_words(const char *path){
    FILE *f=fopen(path,"r"); if(!f){ perror(path); exit(1);}
    char line[256]; int capw=1024; NW=0;
    W=malloc(capw*sizeof(char*)); WB=malloc(capw*4); WS=malloc(capw*4); WL=malloc(capw*4);
    while(fgets(line,sizeof line,f)){
        char w[128]; int n=0;
        for(char*p=line; *p; p++){ int c=*p; if(c>='a'&&c<='z')c-=32;
            if((c>='A'&&c<='Z')||(c>='0'&&c<='9')){ if(n<127) w[n++]=c; } }
        if(!n) continue; w[n]=0;
        if(NW==capw){ capw*=2; W=realloc(W,capw*sizeof(char*)); WB=realloc(WB,capw*4);
            WS=realloc(WS,capw*4); WL=realloc(WL,capw*4); }
        uint32_t h=0,sh=0; for(int i=0;i<n;i++){ int c=w[i]; if(c>='0'&&c<='9')c+=22;
            sh=(sh+(c-64))&31; h^=1u<<sh; }
        W[NW]=strdup(w); WB[NW]=h; WS[NW]=sh; WL[NW]=n; NW++;
    }
    fclose(f);
}

int main(int argc,char**argv){
    const char *wp=NULL,*ip=NULL; int T=0;
    for(int i=1;i<argc;i++){
        if(!strcmp(argv[i],"--depth")&&i+1<argc) g_depth=atoi(argv[++i]);
        else if(!strcmp(argv[i],"--max-overshoot")&&i+1<argc) g_overshoot=atoi(argv[++i]);
        else if(!strcmp(argv[i],"--wordlike")) g_wordlike=1;
        else if(!strcmp(argv[i],"--max-cons")&&i+1<argc) g_maxcons=atoi(argv[++i]);
        else if(!strcmp(argv[i],"--threads")&&i+1<argc) T=atoi(argv[++i]);
        else if(!wp) wp=argv[i]; else if(!ip) ip=argv[i];
    }
    if(!wp||!ip){ fprintf(stderr,"usage: %s [--depth D --max-overshoot N --wordlike --threads T] <words> <ids>\n",argv[0]); return 1; }
    if(T<=0){ long n=sysconf(_SC_NPROCESSORS_ONLN); T=(int)(n>0?n:4); }
    load_words(wp);
    FILE *f=fopen(ip,"r"); if(!f){ perror(ip); return 1;} char line[256]; size_t cnt=0;
    while(fgets(line,sizeof line,f)) if(line[0]&&line[0]!='\n') cnt++;
    rewind(f); set_init(cnt?cnt:1); size_t added=0;
    while(fgets(line,sizeof line,f)){ char*e; unsigned long v=strtoul(line,&e,0); if(e!=line){ set_add((uint32_t)v); added++; } }
    fclose(f);
    fprintf(stderr,"%d words, %zu ids, depth %d, %d threads -> up to %.2g combos\n",
        NW,added,g_depth,T,(double)NW* (g_depth>=2?NW:1) * (g_depth>=3?NW:1));

    if(T>NW) T=NW; if(T<1)T=1;
    pthread_t th[256]; Slice sl[256];
    int per=(NW+T-1)/T;
    for(int t=0;t<T;t++){ sl[t].lo=t*per; sl[t].hi=(t+1)*per>NW?NW:(t+1)*per;
        pthread_create(&th[t],NULL,worker,&sl[t]); }
    for(int t=0;t<T;t++) pthread_join(th[t],NULL);
    fprintf(stderr,"done.\n");
    return 0;
}
