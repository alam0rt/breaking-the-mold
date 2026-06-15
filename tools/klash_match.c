#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * klash_match: read candidate strings on stdin, print the ones whose Skullmonkeys
 * asset id is listed in <ids-file>. Built to sit on the end of a generator pipe:
 *
 *   gcc -O3 -o klash_match tools/klash_match.c
 *   cut -d, -f2 docs/reference/asset-ids-master.csv | tail -n +2 > /tmp/ids.txt
 *   nix-shell -p crunch --run \
 *     "crunch 4 5 ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789" | ./klash_match /tmp/ids.txt
 *
 * Output: "0x<id>\t<candidate>\tfloor=F\tlen=L" per match. The hash ignores case
 * and non-alphanumerics, so any charset/separators in the generator are fine.
 *
 * --max-overshoot N : only accept a hit when (L - floor) is in [0, N]. Each char
 *   past the floor toggles a bit that must be cancelled in pairs, so (L - floor)
 *   is always even; N=0 means "clean" names (effective length == floor), which
 *   drops long-word-on-short-hash collisions (e.g. HILARIOUS on the YES id).
 *   floor = popcount(id ^ 0x28C0E011), derived from each id automatically.
 *   Default: no limit (every collision shown).
 */

static uint32_t rotl(uint32_t v, int r){ r&=31; return (v<<r)|(v>>((32-r)&31)); }
static int popcount32(uint32_t v){ int c=0; while(v){ v&=v-1; c++; } return c; }
/* min name length (floor) for an id, straight from the formula */
static int floor_of(uint32_t id){ return popcount32(id ^ 0x28C0E011u); }

static uint32_t calcHash(const char *s){
    uint32_t h=0, sh=0;
    for(; *s; s++){
        int c=*s;
        if(c>='a'&&c<='z') c-=32; else if(c>='0'&&c<='9') c+=22;
        if((c>='A'&&c<='Z')||(*s>='0'&&*s<='9')){ sh=(sh+(c-64))&31; h^=1u<<sh; }
    }
    return h;
}
static uint32_t asset_id(const char *name){ return 0x28C0E011u ^ rotl(calcHash(name), 27); }
/* effective length = alphanumeric characters only (what the hash counts) */
static int eff_len(const char *s){ int n=0; for(; *s; s++)
    if((*s>='A'&&*s<='Z')||(*s>='a'&&*s<='z')||(*s>='0'&&*s<='9')) n++; return n; }

static int g_maxcons=3;
/* pronounceability heuristic: >=1 vowel, <=50% digits, no long consonant/same run */
static int is_wordlike(const char *s){
    int len=0,vowels=0,digits=0,run=0,same=1; int prev=0;
    for(; *s; s++){
        int c=*s; if(c>='a'&&c<='z') c-=32;
        if(!(((c>='A'&&c<='Z'))||(c>='0'&&c<='9'))) continue;   /* hash ignores it */
        len++;
        int isv=(c=='A'||c=='E'||c=='I'||c=='O'||c=='U');
        int isd=(c>='0'&&c<='9');
        if(isd){ digits++; run=0; }
        else if(isv){ vowels++; run=0; }
        else { if(++run>g_maxcons) return 0; }
        if(c==prev){ if(++same>=3) return 0; } else same=1;
        prev=c;
    }
    return len && vowels && digits*2<=len;
}

/* open-addressing set of target ids; slots == -1 are empty */
static int64_t *slots = NULL; static size_t cap = 0;
static void set_init(size_t n){ cap=16; while(cap<n*2) cap<<=1;
    slots=malloc(cap*sizeof(int64_t)); for(size_t i=0;i<cap;i++) slots[i]=-1; }
static void set_add(uint32_t k){ size_t i=(k*2654435761u)&(cap-1);
    while(slots[i]!=-1){ if((uint32_t)slots[i]==k) return; i=(i+1)&(cap-1);} slots[i]=k; }
static int set_has(uint32_t k){ size_t i=(k*2654435761u)&(cap-1);
    while(slots[i]!=-1){ if((uint32_t)slots[i]==k) return 1; i=(i+1)&(cap-1);} return 0; }

int main(int argc, char **argv){
    const char *ids_path=NULL; int max_overshoot=-1; int wordlike=0; /* -1 = no limit */
    for(int i=1;i<argc;i++){
        if(!strcmp(argv[i],"--max-overshoot") && i+1<argc) max_overshoot=atoi(argv[++i]);
        else if(!strcmp(argv[i],"--wordlike")) wordlike=1;
        else if(!strcmp(argv[i],"--max-cons") && i+1<argc) g_maxcons=atoi(argv[++i]);
        else if(!ids_path) ids_path=argv[i];
    }
    if(!ids_path){ fprintf(stderr,"usage: %s [--max-overshoot N] [--wordlike] [--max-cons C] <ids-file>  (candidates on stdin)\n", argv[0]); return 1; }
    FILE *f=fopen(ids_path,"r"); if(!f){ perror(ids_path); return 1; }
    char line[1024]; size_t n=0;
    while(fgets(line,sizeof line,f)) if(line[0] && line[0]!='\n') n++;
    rewind(f); set_init(n?n:1); size_t added=0;
    while(fgets(line,sizeof line,f)){ char *e; unsigned long v=strtoul(line,&e,0);
        if(e!=line){ set_add((uint32_t)v); added++; } }
    fclose(f);
    fprintf(stderr,"loaded %zu target ids%s", added,
        max_overshoot>=0 ? "" : " (no overshoot filter)\n");
    if(max_overshoot>=0) fprintf(stderr," (accepting len-floor in [0,%d])\n", max_overshoot);

    uint64_t tried=0, hit=0;
    while(fgets(line,sizeof line,stdin)){
        line[strcspn(line,"\r\n")]='\0';
        uint32_t id=asset_id(line);
        if(set_has(id)){
            int fl=floor_of(id), L=eff_len(line), over=L-fl;
            if((max_overshoot<0 || (over>=0 && over<=max_overshoot)) &&
               (!wordlike || is_wordlike(line))){
                printf("0x%08x\t%s\tfloor=%d\tlen=%d\n", id, line, fl, L); hit++;
            }
        }
        if((++tried & 0x3FFFFFFull)==0) fprintf(stderr,"\r%llu tried, %llu hit",
            (unsigned long long)tried,(unsigned long long)hit);
    }
    fprintf(stderr,"\rdone: %llu tried, %llu hit\n",
        (unsigned long long)tried,(unsigned long long)hit);
    return 0;
}
