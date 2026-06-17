/* klash_anchor: find names of the form  PREFIX + ANCHORWORD + SUFFIX  that hash
 * to a target id (RAW or WRAP). The anchor is a known word; PREFIX/SUFFIX are short
 * brute-forced parts. Uses a memoized suffix index (meet-in-the-middle) so we can
 * reach ~13-char names while keeping a real word embedded.
 *
 *   gcc -O3 -pthread -o klash_anchor tools/klash_anchor.c
 *   ./klash_anchor --words anchors.txt --la 3 --lb 4 \
 *        --target 0xa9240484 --target 0x80e85ea0 ...
 *
 * Out: "0x<id> [RAW|WRAP] PREFIX_ANCHOR_SUFFIX   (anchor=ANCHOR)"
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static const char CS[36] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
static int STEP[36];
#define SEED 0x28C0E011u
static inline uint32_t rotl(uint32_t v,int r){ r&=31; return (v<<r)|(v>>((32-r)&31)); }
static inline uint32_t rotr(uint32_t v,int r){ r&=31; return (v>>r)|(v<<((32-r)&31)); }

static void chs(const char*s,uint32_t*ph,int*psh){
    uint32_t h=0; int sh=0;
    for(;*s;s++){ int c=(unsigned char)*s,o;
        if(c>='a'&&c<='z')c-=32;
        if(c>='A'&&c<='Z')o=c; else if(c>='0'&&c<='9')o=c+22; else continue;
        sh=(sh+((o-64)&31))&31; h^=1u<<sh; }
    *ph=h;*psh=sh;
}

/* suffix index: sorted array of (hash, packed-digits, len) for all B len 0..LB */
typedef struct { uint32_t h; uint32_t packed; uint8_t len; } Suf;
static Suf *g_suf=NULL; static uint64_t g_nsuf=0;

static int cmp_suf(const void*a,const void*b){
    uint32_t ha=((const Suf*)a)->h, hb=((const Suf*)b)->h;
    return ha<hb?-1:ha>hb?1:0;
}
static void build_suffixes(int LB){
    uint64_t total=1, p=1; for(int L=1;L<=LB;L++){ p*=36; total+=p; }
    g_suf=malloc(sizeof(Suf)*total); g_nsuf=0;
    /* empty suffix */
    g_suf[g_nsuf++]=(Suf){0,0,0};
    for(int L=1;L<=LB;L++){
        uint64_t cnt=1; for(int i=0;i<L;i++)cnt*=36;
        for(uint64_t n=0;n<cnt;n++){
            uint32_t h=0; int sh=0; uint32_t packed=0; uint64_t x=n;
            for(int i=0;i<L;i++){ int d=x%36; x/=36; sh=(sh+STEP[d])&31; h^=1u<<sh; packed|=(uint32_t)d<<(5*i); }
            g_suf[g_nsuf++]=(Suf){h,packed,(uint8_t)L};
        }
    }
    qsort(g_suf,g_nsuf,sizeof(Suf),cmp_suf);
    fprintf(stderr,"suffix index: %llu entries (len 0..%d)\n",(unsigned long long)g_nsuf,LB);
}
/* lower_bound on hash */
static uint64_t lb_hash(uint32_t key){
    uint64_t lo=0,hi=g_nsuf;
    while(lo<hi){ uint64_t m=(lo+hi)/2; if(g_suf[m].h<key) lo=m+1; else hi=m; }
    return lo;
}
static void unpack(uint32_t packed,int len,char*out){
    for(int i=0;i<len;i++){ out[i]=CS[(packed>>(5*i))&31]; } out[len]=0;
}

#define MAXW 8192
static char g_word[MAXW][40]; static uint32_t g_wh[MAXW]; static int g_wsh[MAXW]; static int NW=0;
#define MAXT 64
static uint32_t g_tgt[MAXT]; static int NT=0;

int main(int argc,char**argv){
    int LA=3, LB=4; const char*wf=NULL;
    for(int i=1;i<argc;i++){
        if(!strcmp(argv[i],"--la")&&i+1<argc)LA=atoi(argv[++i]);
        else if(!strcmp(argv[i],"--lb")&&i+1<argc)LB=atoi(argv[++i]);
        else if(!strcmp(argv[i],"--words")&&i+1<argc)wf=argv[++i];
        else if(!strcmp(argv[i],"--target")&&i+1<argc&&NT<MAXT)g_tgt[NT++]=strtoul(argv[++i],NULL,0);
    }
    if(!wf||NT==0){ fprintf(stderr,"usage: --words FILE --target 0xID [...] [--la 3 --lb 4]\n"); return 1; }
    if(LB>5){fprintf(stderr,"lb<=5\n");return 1;}
    for(int d=0;d<36;d++){ char c=CS[d]; int o=c; if(c>='0'&&c<='9')o+=22; STEP[d]=(o-64)&31; }

    FILE*f=fopen(wf,"r"); if(!f){perror("words");return 1;}
    char line[64];
    while(fgets(line,sizeof line,f)&&NW<MAXW){ char*nl=strchr(line,'\n'); if(nl)*nl=0; if(!line[0])continue;
        strncpy(g_word[NW],line,39); chs(line,&g_wh[NW],&g_wsh[NW]); NW++; }
    fclose(f);
    fprintf(stderr,"%d anchor words, %d targets, LA=%d LB=%d\n",NW,NT,LA,LB);
    build_suffixes(LB);

    /* enumerate prefixes A len 0..LA; for each (target, model, word) MITM lookup */
    uint64_t found=0;
    for(int LAi=0;LAi<=LA;LAi++){
        uint64_t cnt=1; for(int i=0;i<LAi;i++)cnt*=36;
        for(uint64_t n=0;n<cnt;n++){
            uint32_t hA=0; int shA=0; uint8_t dig[8]; uint64_t x=n;
            for(int i=0;i<LAi;i++){ int d=x%36; x/=36; dig[i]=d; shA=(shA+STEP[d])&31; hA^=1u<<shA; }
            for(int w=0;w<NW;w++){
                /* partial after PREFIX+WORD */
                uint32_t P = hA ^ rotl(g_wh[w], shA);
                int shAW = (shA + g_wsh[w]) & 31;
                for(int model=0; model<2; model++){
                    uint32_t Tt;
                    for(int t=0;t<NT;t++){
                        Tt = (model==0)? g_tgt[t] : rotr(g_tgt[t]^SEED,27);
                        /* need rotl(h_B, shAW) = Tt ^ P -> h_B = rotr(Tt^P, shAW) */
                        uint32_t needB = rotr(Tt ^ P, shAW);
                        uint64_t idx=lb_hash(needB);
                        for(;idx<g_nsuf && g_suf[idx].h==needB; idx++){
                            char pre[9],suf[9]; for(int i=0;i<LAi;i++)pre[i]=CS[dig[i]]; pre[LAi]=0;
                            unpack(g_suf[idx].packed,g_suf[idx].len,suf);
                            printf("0x%08x %s %s_%s_%s\t(anchor=%s)\n",
                                g_tgt[t], model?"WRAP":"RAW", pre, g_word[w], suf, g_word[w]);
                            found++;
                        }
                    }
                }
            }
        }
        fprintf(stderr,"  prefixes len %d done; cumulative hits=%llu\n",LAi,(unsigned long long)found);
    }
    fprintf(stderr,"total hits: %llu\n",(unsigned long long)found);
    return 0;
}
