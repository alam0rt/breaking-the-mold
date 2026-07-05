/* =============================================================================
 * menu_spawn_data.c  --  faithful C repro of asm/data/8B944.data.s spawn records
 * =============================================================================
 * D_8009B174 / D_8009B180 / D_8009B18C are 3-word menu/HUD spawn-data records
 * consumed by CreateMenuEntities' three InitEntityWithSprite children (slots
 * 0x25/0x26/0x27). InitEntityWithSprite reads spawn[0] as the sprite id passed
 * to SetEntitySpriteId, so a weak-zeroed record yields sprite id 0 -> the entity
 * gets no frame table and UpdateSpriteFrameData dereferences NULL. These are
 * pure scalar data (sprite id + packed config), transcribed word-for-word from
 * the disassembly. Strong definitions override the weak backing gen_port_stubs
 * emits for the asm("D_...") aliases.
 * ---------------------------------------------------------------------------*/

unsigned int D_8009B174[3] asm("D_8009B174") = { 0x0AD0F813u, 0x69C8F473u, 0x00000000u };
unsigned int D_8009B180[3] asm("D_8009B180") = { 0x69C04050u, 0x2AD0F011u, 0x00000000u };
unsigned int D_8009B18C[3] asm("D_8009B18C") = { 0x68C0F413u, 0x29C0E211u, 0x00000000u };

/* Title-screen menu records consumed by InitMenuStage1 (asm/data/8D3DC.data.s,
 * 8D3F8.data.s). D_8009CBDC[0] is the animated title entity's sprite id;
 * D_8009CBF8 is the part-4 logo spawn record; D_8009CC08 is the NUL-terminated
 * sprite-scan hash list. All weak-zeroed by default, which leaves the title
 * entity spriteless (SetEntitySpriteId(id 0) -> no frame table -> NULL deref in
 * UpdateSpriteFrameData). */
unsigned int D_8009CBDC[3] asm("D_8009CBDC") = { 0x0005C699u, 0x0305A4F5u, 0x00000000u };
unsigned int D_8009CBF8[4] asm("D_8009CBF8") = { 0x40B18011u, 0x40B28011u, 0x40B48011u, 0x00000000u };
unsigned int D_8009CC08[4] asm("D_8009CC08") = { 0x28A0C119u, 0x40B18011u, 0x30A0C119u, 0x00000000u };

