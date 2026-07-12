/* =============================================================================
 * port_hal.h  --  internal glue between the PC-port SPEC backends (TARGET_PC)
 * =============================================================================
 * Not a PSY-Q header. This declares the small set of cross-backend hooks the
 * host frame loop (port_boot.c / host_main.c) uses to drive the HAL, plus
 * shared helper types. The PSY-Q surface itself is declared per-TU by the game
 * and defined in the individual backends; this is only the port's own plumbing.
 * ========================================================================== */
#ifndef PORT_HAL_H
#define PORT_HAL_H

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned long (*PortCallback)();

/* ---- bios.c -------------------------------------------------------------- */
void     port_bios_fire_vsync(void);     /* invoke the registered VSync handler */
void     port_bios_advance_frame(void);  /* bump the vblank counter (1/frame)   */
unsigned port_bios_vblank_count(void);

/* ---- gpu_gl.c ------------------------------------------------------------ */
int  port_gpu_init(void);                /* GL objects; VRAM texture; shaders   */
void port_gpu_shutdown(void);
void port_gpu_begin_frame(void);         /* bind default FBO, set viewport      */
void port_gpu_present(void);             /* swap the SDL GL buffers             */
void port_gpu_set_window(void *sdl_window); /* host_main hands us the window     */
void port_gpu_selftest(void);            /* synthetic 3-primitive OT walk        */

/* ---- pad_sdl.c ----------------------------------------------------------- */
void   port_pad_init(void);
void   port_pad_poll(void);              /* drain SDL input into the pad state  */
int    port_pad_quit_requested(void);    /* window close / ESC               */

/* ---- spu_sdl.c ----------------------------------------------------------- */
int  port_spu_init(void);
void port_spu_shutdown(void);

/* ---- cd_files.c ---------------------------------------------------------- */
int  port_cd_init(void);                 /* locate + open GAME.BLB               */
int  port_cd_read_sectors(int lba, int nsectors, void *buf); /* 0 = ok           */
void port_cd_set_blb_base(int base_lba); /* LBA subtracted from reads (default 0)*/

/* ---- port_heap.c --------------------------------------------------------- */
int  port_heap_init(void);               /* PSX-mirror arena; set g_pBlbHeapBase */
void port_heap_shutdown(void);
unsigned char *port_ram_base(void);      /* host address of PSX 0x80000000       */
void *port_psx2host(unsigned int psx_addr); /* arena address of a PSX RAM addr   */
int  port_ram_mirrored(void);            /* 1 = arena mapped AT 0x80000000       */

/* ---- game_boot.c (port/decomp/boot) -------------------------------------- */
void port_game_boot_init(void);          /* converted main() boot prologue      */
void port_game_boot_frame(void);         /* one converted main() frame iteration */

#ifdef __cplusplus
}
#endif

#endif /* PORT_HAL_H */
