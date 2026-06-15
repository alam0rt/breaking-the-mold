#include <stdint.h>
#include <stdio.h>


/*
 *
klash takes a string and produces the correct 32-bit hash value which was
used to generate asset IDs for Skullmonkeys.

      Worked example: "NO"

      start: h=0, sh=0
       'N' -> step 14 -> sh=14 -> h ^= 1<<14   -> h = 0x00004000
       'O' -> step 15 -> sh=29 -> h ^= 1<<29   -> h = 0x20004000   (= calcHash)
      rotl(0x20004000, 27)              = 0x01000200
      0x01000200 ^ 0x28C0E011          = 0x29C0E211   ✓  (actual id of the "No" sprite)
*/

static uint32_t rotl(uint32_t v, int r){ r&=31; return (v<<r)|(v>>((32-r)&31)); }
static uint32_t rotr(uint32_t v, int r){ return rotl(v, 32-(r&31)); }

uint32_t calcHash(const char *s){
  uint32_t h=0, sh=0; // accumulator and shift
  for(; *s; s++){
      int c=*s;
      if(c>='a'&&c<='z') c-=32; else if(c>='0'&&c<='9') c+=22; // convert to uppercase and remap digits to 27-36
      if((c>='A'&&c<='Z')||(*s>='0'&&*s<='9')){ sh=(sh+(c-64))&31; h^=1u<<sh; }
  }
  return h;
}
uint32_t asset_id(const char *name){ return 0x28C0E011u ^ rotl(calcHash(name), 27); }

int main(){
    char line[256];

    while(fgets(line, sizeof(line), stdin)){
        uint32_t res=asset_id(line);
        printf("0x%08x\n", res);
    }
    return 0;
}
