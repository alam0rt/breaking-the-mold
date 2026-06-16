/* kcrack — Skullmonkeys (SLES-01090) asset-name cracker, consolidated.
 *
 * Hash (TWO NAMESPACES recovered):
 *   menu:  asset_id(name) = 0x28C0E011 ^ rotl(calcHash(name), 27)   [default]
 *   blb :  asset_id(name) = calcHash(name)                           [--raw]
 *   calcHash: each alphanumeric char advances a 5-bit running position and
 *   toggles that bit. Case-insensitive; non-alphanumerics are ignored.
 *
 * Use --raw on every BLB asset id (sprite/anim/audio). Only use the default
 * (wrap) on UI/menu strings (NO/YES/PAUSED/QUIT/CONTINUE/QUITGAME).
 *
 * Knowledge baked in:
 *   - floor(id) = popcount(id ^ SEED) = minimum name length (lower bound; true
 *     length has the SAME parity and is floor + 2k for k internal collisions).
 *   - a length-L string can only hit an id with floor F when F<=L and (L-F) even;
 *     --overshoot N keeps only hits with (L - floor) in [0,N] (N=0 = clean).
 *   - --wordlike drops unpronounceable gibberish (vowel / digit / run heuristic).
 *   - FAMILY UNLOCK: siblings share a name root, so once one name is known the
 *     rest differ by a short affix -> 'extend' brutes only that affix (ms, not eons).
 *
 * Build:  gcc -O3 -pthread -o kcrack tools/kcrack.c
 *
 * Modes:
 *   kcrack hash [STR...]                      # STR or stdin -> id
 *   kcrack match  [opts] IDS                  # stdin candidates -> hits in IDS
 *   kcrack combine[opts] WORDS IDS  --depth D # every <=D-word combo -> hits
 *   kcrack extend [opts] ROOT  IDS            # ROOT + brute affix -> hits (family unlock)
 *
 * opts: --overshoot N  --wordlike  --max-cons C  --threads T
 *       extend only:  --suf S (max suffix chars, def 4)  --pre P (max prefix chars, def 0)
 *       combine only: --depth D (def 3)
 * Output: 0x<id>\t<string>\tfloor=F\tlen=L   (flushed)
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#define SEED 0x28C0E011u
static int g_raw = 0;                  /* 1 = use raw calcHash (BLB namespace) */
static const char CS[36] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
static int STEPC[36];                 /* per-symbol calcHash step */

static uint32_t rotl(uint32_t v,int r){ r&=31; return (v<<r)|(v>>((32-r)&31)); }
static int popc(uint32_t v){ int c=0; while(v){v&=v-1;c++;} return c; }
static int floor_of(uint32_t id){ return g_raw ? popc(id) : popc(id ^ SEED); }
static uint32_t wrap_or_raw(uint32_t h){ return g_raw ? h : (SEED ^ rotl(h,27)); }

/* full calcHash of a C string (handles case + ignores non-alnum) */
static uint32_t calc(const char *s){
    uint32_t h=0,sh=0;
    for(; *s; s++){ int c=*s; if(c>='a'&&c<='z')c-=32; else if(c>='0'&&c<='9')c+=22;
        if((c>='A'&&c<='Z')||(*s>='0'&&*s<='9')){ sh=(sh+(c-64))&31; h^=1u<<sh; } }
    return h;
}
/* calcHash state (h + running pos) for incremental appending */
static void calc_state(const char *s, uint32_t *h, uint32_t *sh){
    uint32_t hh=*h, ss=*sh;
    for(; *s; s++){ int c=*s; if(c>='a'&&c<='z')c-=32; else if(c>='0'&&c<='9')c+=22;
        if((c>='A'&&c<='Z')||(*s>='0'&&*s<='9')){ ss=(ss+(c-64))&31; hh^=1u<<ss; } }
    *h=hh; *sh=ss;
}
static uint32_t asset_id(const char *s){ return wrap_or_raw(calc(s)); }
static int eff_len(const char *s){ int n=0; for(; *s; s++)
    if((*s>='A'&&*s<='Z')||(*s>='a'&&*s<='z')||(*s>='0'&&*s<='9')) n++; return n; }

/* ---- target id set (open addressing, -1 empty) ---- */
static int64_t *slots=NULL; static size_t cap=0;
static void set_init(size_t n){ cap=16; while(cap<n*2) cap<<=1;
    slots=malloc(cap*sizeof(int64_t)); for(size_t i=0;i<cap;i++) slots[i]=-1; }
static void set_add(uint32_t k){ size_t i=(k*2654435761u)&(cap-1);
    while(slots[i]!=-1){ if((uint32_t)slots[i]==k) return; i=(i+1)&(cap-1);} slots[i]=k; }
static inline int set_has(uint32_t k){ size_t i=(k*2654435761u)&(cap-1);
    while(slots[i]!=-1){ if((uint32_t)slots[i]==k) return 1; i=(i+1)&(cap-1);} return 0; }
static size_t load_ids(const char *path){
    FILE *f=fopen(path,"r"); if(!f){ perror(path); exit(1);} char line[256]; size_t n=0;
    while(fgets(line,sizeof line,f)) if(line[0]&&line[0]!='\n') n++;
    rewind(f); set_init(n?n:1); size_t a=0;
    while(fgets(line,sizeof line,f)){ char*e; unsigned long v=strtoul(line,&e,0); if(e!=line){ set_add((uint32_t)v); a++; } }
    fclose(f); return a;
}

/* ---- shared options + reporting ---- */
static int g_overshoot=-1, g_wordlike=0, g_maxcons=3, g_threads=0;
static pthread_mutex_t outmu=PTHREAD_MUTEX_INITIALIZER;
static int wordlike(const char *s){
    int len=0,vow=0,dig=0,run=0,same=1,prev=0;
    for(; *s; s++){ int c=*s; if(c>='a'&&c<='z')c-=32;
        if(!(((c>='A'&&c<='Z'))||(c>='0'&&c<='9'))){ run=0; same=1; prev=0; continue; }
        len++; int isv=(c=='A'||c=='E'||c=='I'||c=='O'||c=='U'); int isd=(c>='0'&&c<='9');
        if(isd){dig++;run=0;} else if(isv){vow++;run=0;} else { if(++run>g_maxcons) return 0; }
        if(c==prev){ if(++same>=3) return 0; } else same=1; prev=c; }
    return len&&vow&&dig*2<=len;
}
static void report(const char *s, uint32_t id){
    int fl=floor_of(id), L=eff_len(s), over=L-fl;
    if(g_overshoot>=0 && (over<0||over>g_overshoot)) return;
    if(g_wordlike && !wordlike(s)) return;
    pthread_mutex_lock(&outmu);
    printf("0x%08x\t%s\tfloor=%d\tlen=%d\n", id, s, fl, L); fflush(stdout);
    pthread_mutex_unlock(&outmu);
}
static int nthreads(void){ if(g_threads>0) return g_threads;
    long n=sysconf(_SC_NPROCESSORS_ONLN); return (int)(n>0?n:4); }

/* ============================ mode: hash ============================ */
static int do_hash(int argc,char**argv,int i){
    int printed=0;
    for(int j=i; j<argc; j++){
        if(argv[j][0]=='-' && argv[j][1]=='-'){ /* skip flags & their values */
            if(!strcmp(argv[j],"--overshoot")||!strcmp(argv[j],"--max-cons")
              ||!strcmp(argv[j],"--threads")||!strcmp(argv[j],"--depth")
              ||!strcmp(argv[j],"--suf")||!strcmp(argv[j],"--pre")) j++;
            continue;
        }
        printf("0x%08x\t%s\n", asset_id(argv[j]), argv[j]); printed=1;
    }
    if(!printed){ char line[256]; while(fgets(line,sizeof line,stdin)){ line[strcspn(line,"\r\n")]=0;
        printf("0x%08x\n", asset_id(line)); } }
    return 0;
}

/* ============================ mode: match ============================ */
static int do_match(const char *ids){
    fprintf(stderr,"loaded %zu ids\n", load_ids(ids));
    char line[1024]; while(fgets(line,sizeof line,stdin)){ line[strcspn(line,"\r\n")]=0;
        uint32_t id=asset_id(line); if(set_has(id)) report(line,id); }
    return 0;
}

/* ========================== mode: combine ========================== */
static char **CW=NULL; static uint32_t *CB=NULL; static int *CSx=NULL,*CL=NULL,CN=0,g_depth=3;
static void load_words(const char *p){
    FILE *f=fopen(p,"r"); if(!f){ perror(p); exit(1);} char line[256]; int capn=1024; CN=0;
    CW=malloc(capn*sizeof(char*)); CB=malloc(capn*4); CSx=malloc(capn*4); CL=malloc(capn*4);
    while(fgets(line,sizeof line,f)){ char w[128]; int n=0;
        for(char*q=line;*q;q++){ int c=*q; if(c>='a'&&c<='z')c-=32;
            if((c>='A'&&c<='Z')||(c>='0'&&c<='9')){ if(n<127) w[n++]=c; } }
        if(!n) continue; w[n]=0;
        if(CN==capn){ capn*=2; CW=realloc(CW,capn*sizeof(char*)); CB=realloc(CB,capn*4); CSx=realloc(CSx,capn*4); CL=realloc(CL,capn*4); }
        uint32_t h=0,sh=0; calc_state(w,&h,&sh);
        CW[CN]=strdup(w); CB[CN]=h; CSx[CN]=sh; CL[CN]=n; CN++; }
    fclose(f);
}
static void emit_combo(int *idx,int depth,uint32_t id){
    char buf[256]; int p=0; for(int d=0;d<depth;d++){ if(d) buf[p++]='_';
        memcpy(buf+p,CW[idx[d]],CL[idx[d]]); p+=CL[idx[d]]; } buf[p]=0; report(buf,id);
}
static void rec_combo(int depth,uint32_t h,uint32_t sh,int *idx){
    uint32_t id=wrap_or_raw(h); if(set_has(id)) emit_combo(idx,depth,id);
    if(depth>=g_depth) return;
    for(int w=0;w<CN;w++){ idx[depth]=w; rec_combo(depth+1, h^rotl(CB[w],sh),(sh+CSx[w])&31,idx); }
}
typedef struct { int lo,hi; } Slice;
static void *combo_worker(void *a){ Slice*s=(Slice*)a; int idx[16];
    for(int w=s->lo;w<s->hi;w++){ idx[0]=w; rec_combo(1,CB[w],CSx[w],idx); } return NULL; }
static int do_combine(const char *words,const char *ids){
    load_words(words); size_t ni=load_ids(ids); int T=nthreads(); if(T>CN)T=CN; if(T<1)T=1;
    fprintf(stderr,"%d words, %zu ids, depth %d, %d threads\n",CN,ni,g_depth,T);
    pthread_t th[256]; Slice sl[256]; int per=(CN+T-1)/T;
    for(int t=0;t<T;t++){ sl[t].lo=t*per; sl[t].hi=(t+1)*per>CN?CN:(t+1)*per; pthread_create(&th[t],NULL,combo_worker,&sl[t]); }
    for(int t=0;t<T;t++) pthread_join(th[t],NULL); fprintf(stderr,"done.\n"); return 0;
}

/* =========================== mode: extend ===========================
 * Known ROOT + affix brute. Default: ROOT + suffix(<=S). With --pre P also
 * tries prefix(<=P) + ROOT. This is the family-unlock: give it a confirmed
 * name and the id list, it finds that name's short-affix siblings.        */
static const char *g_root; static int g_suf=4, g_pre=0;
static uint32_t g_hbase, g_shbase;   /* calc state of prefix+ROOT for current pass */
static char g_prefix[64];
static void emit_ext(const char *suffix,uint32_t id){
    char buf[256]; snprintf(buf,sizeof buf,"%s%s%s",g_prefix,g_root,suffix); report(buf,id);
}
static void rec_suf(int depth,uint32_t h,uint32_t sh,char *suf){
    uint32_t id=wrap_or_raw(h); if(set_has(id)){ suf[depth]=0; emit_ext(suf,id); }
    if(depth>=g_suf) return;
    for(int d=0;d<36;d++){ uint32_t nsh=(sh+STEPC[d])&31; suf[depth]=CS[d];
        rec_suf(depth+1, h^(1u<<nsh), nsh, suf); }
}
typedef struct { int lo,hi; } ESlice;
static void *suf_worker(void *a){ ESlice*s=(ESlice*)a; char suf[32];
    for(int d=s->lo;d<s->hi;d++){ uint32_t nsh=(g_shbase+STEPC[d])&31; suf[0]=CS[d];
        rec_suf(1, g_hbase^(1u<<nsh), nsh, suf); } return NULL; }
static void brute_suffix(void){           /* uses current g_hbase/g_shbase/g_prefix */
    char none[1]={0}; uint32_t id0=wrap_or_raw(g_hbase);   /* ROOT alone (suffix len 0) */
    if(set_has(id0)) emit_ext(none,id0);
    if(g_suf<1) return;
    int T=nthreads(); if(T>36)T=36; pthread_t th[36]; ESlice sl[36]; int per=(36+T-1)/T;
    for(int t=0;t<T;t++){ sl[t].lo=t*per; sl[t].hi=(t+1)*per>36?36:(t+1)*per; pthread_create(&th[t],NULL,suf_worker,&sl[t]); }
    for(int t=0;t<T;t++) pthread_join(th[t],NULL);
}
static void each_prefix(int depth,uint32_t h,uint32_t sh){   /* enumerate prefixes 0..g_pre */
    g_prefix[depth]=0;                                       /* current prefix string */
    uint32_t hr=h, shr=sh; { uint32_t rh=hr,rs=shr; calc_state(g_root,&rh,&rs); g_hbase=rh; g_shbase=rs; }
    brute_suffix();
    if(depth>=g_pre) return;
    for(int d=0;d<36;d++){ uint32_t nsh=(sh+STEPC[d])&31; g_prefix[depth]=CS[d];
        each_prefix(depth+1, h^(1u<<nsh), nsh); }
}
static int do_extend(const char *root,const char *ids){
    g_root=root; size_t ni=load_ids(ids);
    fprintf(stderr,"root '%s' (id 0x%08x), %zu ids, suffix<=%d prefix<=%d, %d threads\n",
        root, asset_id(root), ni, g_suf, g_pre, nthreads());
    each_prefix(0,0,0); fprintf(stderr,"done.\n"); return 0;
}

/* ============================== main ============================== */
int main(int argc,char**argv){
    for(int d=0;d<36;d++){ int c=CS[d]; if(c>='0'&&c<='9')c+=22; STEPC[d]=(c-64)&31; }
    if(argc<2){ fprintf(stderr,"usage: kcrack hash|match|combine|extend ...\n"); return 1; }
    const char *mode=argv[1]; const char *pos[4]; int np=0;
    for(int i=2;i<argc;i++){
        if(!strcmp(argv[i],"--overshoot")&&i+1<argc) g_overshoot=atoi(argv[++i]);
        else if(!strcmp(argv[i],"--wordlike")) g_wordlike=1;
        else if(!strcmp(argv[i],"--max-cons")&&i+1<argc) g_maxcons=atoi(argv[++i]);
        else if(!strcmp(argv[i],"--threads")&&i+1<argc) g_threads=atoi(argv[++i]);
        else if(!strcmp(argv[i],"--depth")&&i+1<argc) g_depth=atoi(argv[++i]);
        else if(!strcmp(argv[i],"--suf")&&i+1<argc) g_suf=atoi(argv[++i]);
        else if(!strcmp(argv[i],"--pre")&&i+1<argc) g_pre=atoi(argv[++i]);
        else if(!strcmp(argv[i],"--raw")) g_raw=1;
        else if(np<4) pos[np++]=argv[i];
    }
    if(!strcmp(mode,"hash"))    return do_hash(argc,argv,2);
    if(!strcmp(mode,"match"))   { if(np<1){fprintf(stderr,"match needs IDS\n");return 1;} return do_match(pos[0]); }
    if(!strcmp(mode,"combine")) { if(np<2){fprintf(stderr,"combine needs WORDS IDS\n");return 1;} return do_combine(pos[0],pos[1]); }
    if(!strcmp(mode,"extend"))  { if(np<2){fprintf(stderr,"extend needs ROOT IDS\n");return 1;} return do_extend(pos[0],pos[1]); }
    fprintf(stderr,"unknown mode '%s'\n",mode); return 1;
}
