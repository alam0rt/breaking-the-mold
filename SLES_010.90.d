build/SLES_010.90.elf: \
    build/asm/header.o \
    build/asm/crt0.o \
    build/src/crt0stub.o \
    build/asm/data/Game/ENGINE.rodata.o \
    build/asm/data/Game/OBJECT.rodata.o \
    build/asm/data/Game/BOSS.rodata.o \
    build/asm/data/Game/PLAYER.rodata.o \
    build/asm/data/Game/PLAYER_STATES.rodata.o \
    build/asm/data/Game/VEHICLE/vehicle.rodata.o \
    build/asm/data/Game/MAIN.rodata.o \
    build/asm/data/LIBCD.rodata.o \
    build/asm/data/LIBGPU.rodata.o \
    build/asm/data/LIBSPU.rodata.o \
    build/src/gfx.o \
    build/src/prim.o \
    build/asm/vram.o \
    build/src/vibrate.o \
    build/asm/sprite.o \
    build/src/spracc.o \
    build/src/nullfn.o \
    build/asm/layer.o \
    build/src/entity.o \
    build/src/sprset.o \
    build/src/anim.o \
    build/src/blb.o \
    build/asm/hud.o \
    build/asm/edtor.o \
    build/asm/decor.o \
    build/asm/pickups.o \
    build/asm/effects.o \
    build/asm/gamecd.o \
    build/asm/movie.o \
    build/asm/enemies.o \
    build/asm/bosses.o \
    build/src/Game/BOSS/clayball_platform.o \
    build/src/Game/PLAYER/player.o \
    build/asm/Game/PLAYER/finn.o \
    build/src/Game/PLAYER/destructor_spu_at10c.o \
    build/src/Game/PLAYER_STATES/player_states.o \
    build/asm/Game/UI/menu.o \
    build/asm/Game/UI/password.o \
    build/asm/Game/UI/hud_results.o \
    build/asm/Game/UI/ending.o \
    build/asm/Game/MAIN/level.o \
    build/asm/Game/MAIN/blb_accessors.o \
    build/asm/Game/AUDIO/sound.o \
    build/asm/Game/MAIN/gamestate.o \
    build/src/Game/MAIN/level_load.o \
    build/src/Game/MAIN/entity_init.o \
    build/src/Game/MAIN/main.o \
    build/src/Game/VEHICLE/static_game_state.o \
    build/src/Game/VEHICLE/empty_callbacks.o \
    build/src/Game/MAIN/blb_memory.o \
    build/src/Game/MAIN/memmove.o \
    build/src/LIBCD/libcd.o \
    build/src/LIBSPU/libspu_voice.o \
    build/asm/LIBSPU_tail.o \
    build/asm/Game/PLAYER.o \
    build/asm/Game/MAIN.o \
    build/asm/LIBGPU.o \
    build/asm/LIBSPU.o \
    build/asm/data/80FEC.data.o \
    build/asm/data/96154.sdata.o \
    build/asm/data/96920.sbss.o \
    build/asm/data/96920.bss.o
build/asm/header.o:
build/asm/crt0.o:
build/src/crt0stub.o:
build/asm/data/Game/ENGINE.rodata.o:
build/asm/data/Game/OBJECT.rodata.o:
build/asm/data/Game/BOSS.rodata.o:
build/asm/data/Game/PLAYER.rodata.o:
build/asm/data/Game/PLAYER_STATES.rodata.o:
build/asm/data/Game/VEHICLE/vehicle.rodata.o:
build/asm/data/Game/MAIN.rodata.o:
build/asm/data/LIBCD.rodata.o:
build/asm/data/LIBGPU.rodata.o:
build/asm/data/LIBSPU.rodata.o:
build/src/gfx.o:
build/src/prim.o:
build/asm/vram.o:
build/src/vibrate.o:
build/asm/sprite.o:
build/src/spracc.o:
build/src/nullfn.o:
build/asm/layer.o:
build/src/entity.o:
build/src/sprset.o:
build/src/anim.o:
build/src/blb.o:
build/asm/hud.o:
build/asm/edtor.o:
build/asm/decor.o:
build/asm/pickups.o:
build/asm/effects.o:
build/asm/gamecd.o:
build/asm/movie.o:
build/asm/enemies.o:
build/asm/bosses.o:
build/src/Game/BOSS/clayball_platform.o:
build/src/Game/PLAYER/player.o:
build/asm/Game/PLAYER/finn.o:
build/src/Game/PLAYER/destructor_spu_at10c.o:
build/src/Game/PLAYER_STATES/player_states.o:
build/asm/Game/UI/menu.o:
build/asm/Game/UI/password.o:
build/asm/Game/UI/hud_results.o:
build/asm/Game/UI/ending.o:
build/asm/Game/MAIN/level.o:
build/asm/Game/MAIN/blb_accessors.o:
build/asm/Game/AUDIO/sound.o:
build/asm/Game/MAIN/gamestate.o:
build/src/Game/MAIN/level_load.o:
build/src/Game/MAIN/entity_init.o:
build/src/Game/MAIN/main.o:
build/src/Game/VEHICLE/static_game_state.o:
build/src/Game/VEHICLE/empty_callbacks.o:
build/src/Game/MAIN/blb_memory.o:
build/src/Game/MAIN/memmove.o:
build/src/LIBCD/libcd.o:
build/src/LIBSPU/libspu_voice.o:
build/asm/LIBSPU_tail.o:
build/asm/Game/PLAYER.o:
build/asm/Game/MAIN.o:
build/asm/LIBGPU.o:
build/asm/LIBSPU.o:
build/asm/data/80FEC.data.o:
build/asm/data/96154.sdata.o:
build/asm/data/96920.sbss.o:
build/asm/data/96920.bss.o:
-include build/asm/header.d build/asm/crt0.d build/src/crt0stub.d build/asm/data/Game/ENGINE.rodata.d build/asm/data/Game/OBJECT.rodata.d build/asm/data/Game/BOSS.rodata.d build/asm/data/Game/PLAYER.rodata.d build/asm/data/Game/PLAYER_STATES.rodata.d build/asm/data/Game/VEHICLE/vehicle.rodata.d build/asm/data/Game/MAIN.rodata.d build/asm/data/LIBCD.rodata.d build/asm/data/LIBGPU.rodata.d build/asm/data/LIBSPU.rodata.d build/src/gfx.d build/src/prim.d build/asm/vram.d build/src/vibrate.d build/asm/sprite.d build/src/spracc.d build/src/nullfn.d build/asm/layer.d build/src/entity.d build/src/sprset.d build/src/anim.d build/src/blb.d build/asm/hud.d build/asm/edtor.d build/asm/decor.d build/asm/pickups.d build/asm/effects.d build/asm/gamecd.d build/asm/movie.d build/asm/enemies.d build/asm/bosses.d build/src/Game/BOSS/clayball_platform.d build/src/Game/PLAYER/player.d build/asm/Game/PLAYER/finn.d build/src/Game/PLAYER/destructor_spu_at10c.d build/src/Game/PLAYER_STATES/player_states.d build/asm/Game/UI/menu.d build/asm/Game/UI/password.d build/asm/Game/UI/hud_results.d build/asm/Game/UI/ending.d build/asm/Game/MAIN/level.d build/asm/Game/MAIN/blb_accessors.d build/asm/Game/AUDIO/sound.d build/asm/Game/MAIN/gamestate.d build/src/Game/MAIN/level_load.d build/src/Game/MAIN/entity_init.d build/src/Game/MAIN/main.d build/src/Game/VEHICLE/static_game_state.d build/src/Game/VEHICLE/empty_callbacks.d build/src/Game/MAIN/blb_memory.d build/src/Game/MAIN/memmove.d build/src/LIBCD/libcd.d build/src/LIBSPU/libspu_voice.d build/asm/LIBSPU_tail.d build/asm/Game/PLAYER.d build/asm/Game/MAIN.d build/asm/LIBGPU.d build/asm/LIBSPU.d build/asm/data/80FEC.data.d build/asm/data/96154.sdata.d build/asm/data/96920.sbss.d build/asm/data/96920.bss.d
