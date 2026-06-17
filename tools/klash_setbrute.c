/* klash_setbrute: brute all short strings once, check calcHash against a SET of
 * target hashes (raw-equivalents). Targets file: one hex per line. For WRAP targets
 * pass rotr(id ^ 0x28C0E011, 27) as the raw-equivalent.
 *   gcc -O3 -pthread -o klash_setbrute tools/klash_setbrute.c
 *   ./klash_setbrute --targets t.txt --min 4 --max 7 [--wordlike]
 * Output: "<string>\t0x<calcHash>"
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

static const char CS[36]="ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
static int STEP[36];
static uint32_t *T=NULL; static int NT=0;
static int g_wordlike=0, g_maxcons=3;
static pthread_mutex_t mu=PTHREAD_MUTEX_INITIALIZER;

static int cmpu(const void*a,const void*b){uint32_t x=*(uint32_t*)a,y=*(uint32_t*)b;return x<y?-1:x>y?1:0;}
static int inset(uint32_t h){int lo=0,hi=NT;while(lo<hi){int m=(lo+hi)/2;if(T[m]<h)lo=m+1;else if(T[m]>h)hi=m;else return 1;}return 0;}

static int wordlike(const uint8_t*d,int L){
    int v=0,run=0,same=1;uint8_t prev=255;
    for(int i=0;i<L;i++){char c=CS[d[i]];int isv=(c=='A'||c=='E'||c=='I'||c=='O'||c=='U'||c=='Y');int isd=(c>='0'&&c<='9');
        if(isd)run=0;else if(isv){v++;run=0;}else{if(++run>g_maxcons)return 0;}
        if(d[i]==prev){if(++same>=3)return 0;}else same=1;prev=d[i];}
    return v>0;
}

typedef struct{int L;uint64_t start,count;}Job;
static void*worker(void*arg){Job*j=(Job*)arg;int L=j->L;uint8_t d[16]={0};uint64_t idx=j->start;
    for(int p=0;p<L;p++){d[p]=idx%36;idx/=36;}
    for(uint64_t n=0;n<j->count;n++){
        uint32_t h=0,sh=0;for(int p=0;p<L;p++){sh=(sh+STEP[d[p]])&31;h^=1u<<sh;}
        if(inset(h)){
            if(!g_wordlike||wordlike(d,L)){
                char s[17];for(int p=0;p<L;p++)s[p]=CS[d[p]];s[L]=0;
                pthread_mutex_lock(&mu);printf("%s\t0x%08x\n",s,h);fflush(stdout);pthread_mutex_unlock(&mu);}
        }
        int p=0;while(p<L&&++d[p]==36){d[p]=0;p++;}
    }
    return NULL;
}
static uint64_t ip36(int L){uint64_t v=1;for(int i=0;i<L;i++)v*=36;return v;}

int main(int argc,char**argv){
    int lmin=4,lmax=7,TH=0;const char*tf=NULL;
    for(int i=1;i<argc;i++){
        if(!strcmp(argv[i],"--min"))lmin=atoi(argv[++i]);
        else if(!strcmp(argv[i],"--max"))lmax=atoi(argv[++i]);
        else if(!strcmp(argv[i],"--threads"))TH=atoi(argv[++i]);
        else if(!strcmp(argv[i],"--targets"))tf=argv[++i];
        else if(!strcmp(argv[i],"--wordlike"))g_wordlike=1;
    }
    if(!tf){fprintf(stderr,"need --targets file\n");return 1;}
    if(lmax>8){fprintf(stderr,"max<=8\n");return 1;}
    for(int d=0;d<36;d++){char c=CS[d];int o=c;if(c>='0'&&c<='9')o+=22;STEP[d]=(o-64)&31;}
    FILE*f=fopen(tf,"r");if(!f){perror("targets");return 1;}
    uint32_t*buf=malloc(sizeof(uint32_t)*100000);char line[64];
    while(fgets(line,sizeof line,f))if(line[0])buf[NT++]=strtoul(line,NULL,0);
    fclose(f);qsort(buf,NT,sizeof(uint32_t),cmpu);T=buf;
    fprintf(stderr,"%d targets, lengths %d..%d, wordlike=%d\n",NT,lmin,lmax,g_wordlike);
    if(TH<=0){long n=sysconf(_SC_NPROCESSORS_ONLN);TH=n>0?n:4;}
    pthread_t*th=malloc(sizeof(pthread_t)*TH);Job*jb=malloc(sizeof(Job)*TH);
    for(int L=lmin;L<=lmax;L++){uint64_t tot=ip36(L),ch=(tot+TH-1)/TH;
        for(int t=0;t<TH;t++){uint64_t s=(uint64_t)t*ch;jb[t].L=L;jb[t].start=s;jb[t].count=s>=tot?0:(s+ch>tot?tot-s:ch);pthread_create(&th[t],NULL,worker,&jb[t]);}
        for(int t=0;t<TH;t++)pthread_join(th[t],NULL);
        fprintf(stderr,"  len %d done\n",L);}
    return 0;
}
