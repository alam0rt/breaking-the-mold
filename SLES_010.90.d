build/SLES_010.90.elf: \
    build/asm/header.o \
    build/asm/Game/ENGINE_boot.o \
    build/src/Game/ENGINE/early_stub.o \
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
    build/src/Game/ENGINE/prim_alloc.o \
    build/asm/Game/ENGINE_40F0.o \
    build/src/Game/ENGINE/stub_vibrate_off.o \
    build/asm/Game/ENGINE_5C3C.o \
    build/src/Game/ENGINE/sprite_accessors.o \
    build/src/Game/ENGINE/empty_stub_18d4c.o \
    build/asm/Game/ENGINE_9554.o \
    build/src/Game/ENGINE/entity_system.o \
    build/src/Game/ENGINE/sprite_setters.o \
    build/src/Game/ENGINE/animation_setters.o \
    build/asm/Game/OBJECT/hud.o \
    build/asm/Game/OBJECT/entity_dtor.o \
    build/asm/Game/OBJECT/decor.o \
    build/asm/Game/OBJECT/collectibles.o \
    build/asm/Game/OBJECT/effects.o \
    build/asm/Game/OBJECT/cd.o \
    build/asm/Game/OBJECT/movie.o \
    build/asm/Game/OBJECT/enemies.o \
    build/asm/Game/OBJECT/bosses.o \
    build/src/Game/BOSS/boss.o \
    build/asm/Game/PLAYER/finn.o \
    build/src/Game/PLAYER/destructor_spu_at10c.o \
    build/src/Game/PLAYER_STATES/player_states.o \
    build/src/Game/MAIN/gamestate.o \
    build/src/Game/VEHICLE/static_game_state.o \
    build/src/Game/VEHICLE/empty_callbacks.o \
    build/src/Game/MAIN/blb_memory.o \
    build/src/Game/MAIN/memmove.o \
    build/src/LIBCD/libcd.o \
    build/src/LIBSPU/libspu_voice.o \
    build/asm/LIBSPU_tail.o \
    build/src/Game/ENGINE.o \
    build/asm/Game/PLAYER.o \
    build/src/Game/VEHICLE/vehicle.o \
    build/asm/Game/MAIN.o \
    build/asm/LIBGPU.o \
    build/asm/LIBSPU.o \
    build/asm/data/80FEC.data.o \
    build/asm/data/96154.sdata.o \
    build/asm/data/96920.sbss.o \
    build/asm/data/96920.bss.o
build/asm/header.o:
build/asm/Game/ENGINE_boot.o:
build/src/Game/ENGINE/early_stub.o:
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
build/src/Game/ENGINE/prim_alloc.o:
build/asm/Game/ENGINE_40F0.o:
build/src/Game/ENGINE/stub_vibrate_off.o:
build/asm/Game/ENGINE_5C3C.o:
build/src/Game/ENGINE/sprite_accessors.o:
build/src/Game/ENGINE/empty_stub_18d4c.o:
build/asm/Game/ENGINE_9554.o:
build/src/Game/ENGINE/entity_system.o:
build/src/Game/ENGINE/sprite_setters.o:
build/src/Game/ENGINE/animation_setters.o:
build/asm/Game/OBJECT/hud.o:
build/asm/Game/OBJECT/entity_dtor.o:
build/asm/Game/OBJECT/decor.o:
build/asm/Game/OBJECT/collectibles.o:
build/asm/Game/OBJECT/effects.o:
build/asm/Game/OBJECT/cd.o:
build/asm/Game/OBJECT/movie.o:
build/asm/Game/OBJECT/enemies.o:
build/asm/Game/OBJECT/bosses.o:
build/src/Game/BOSS/boss.o:
build/asm/Game/PLAYER/finn.o:
build/src/Game/PLAYER/destructor_spu_at10c.o:
build/src/Game/PLAYER_STATES/player_states.o:
build/src/Game/MAIN/gamestate.o:
build/src/Game/VEHICLE/static_game_state.o:
build/src/Game/VEHICLE/empty_callbacks.o:
build/src/Game/MAIN/blb_memory.o:
build/src/Game/MAIN/memmove.o:
build/src/LIBCD/libcd.o:
build/src/LIBSPU/libspu_voice.o:
build/asm/LIBSPU_tail.o:
build/src/Game/ENGINE.o:
build/asm/Game/PLAYER.o:
build/src/Game/VEHICLE/vehicle.o:
build/asm/Game/MAIN.o:
build/asm/LIBGPU.o:
build/asm/LIBSPU.o:
build/asm/data/80FEC.data.o:
build/asm/data/96154.sdata.o:
build/asm/data/96920.sbss.o:
build/asm/data/96920.bss.o:
-include build/asm/header.d build/asm/Game/ENGINE_boot.d build/src/Game/ENGINE/early_stub.d build/asm/data/Game/ENGINE.rodata.d build/asm/data/Game/OBJECT.rodata.d build/asm/data/Game/BOSS.rodata.d build/asm/data/Game/PLAYER.rodata.d build/asm/data/Game/PLAYER_STATES.rodata.d build/asm/data/Game/VEHICLE/vehicle.rodata.d build/asm/data/Game/MAIN.rodata.d build/asm/data/LIBCD.rodata.d build/asm/data/LIBGPU.rodata.d build/asm/data/LIBSPU.rodata.d build/src/Game/ENGINE/prim_alloc.d build/asm/Game/ENGINE_40F0.d build/src/Game/ENGINE/stub_vibrate_off.d build/asm/Game/ENGINE_5C3C.d build/src/Game/ENGINE/sprite_accessors.d build/src/Game/ENGINE/empty_stub_18d4c.d build/asm/Game/ENGINE_9554.d build/src/Game/ENGINE/entity_system.d build/src/Game/ENGINE/sprite_setters.d build/src/Game/ENGINE/animation_setters.d build/asm/Game/OBJECT/hud.d build/asm/Game/OBJECT/entity_dtor.d build/asm/Game/OBJECT/decor.d build/asm/Game/OBJECT/collectibles.d build/asm/Game/OBJECT/effects.d build/asm/Game/OBJECT/cd.d build/asm/Game/OBJECT/movie.d build/asm/Game/OBJECT/enemies.d build/asm/Game/OBJECT/bosses.d build/src/Game/BOSS/boss.d build/asm/Game/PLAYER/finn.d build/src/Game/PLAYER/destructor_spu_at10c.d build/src/Game/PLAYER_STATES/player_states.d build/src/Game/MAIN/gamestate.d build/src/Game/VEHICLE/static_game_state.d build/src/Game/VEHICLE/empty_callbacks.d build/src/Game/MAIN/blb_memory.d build/src/Game/MAIN/memmove.d build/src/LIBCD/libcd.d build/src/LIBSPU/libspu_voice.d build/asm/LIBSPU_tail.d build/src/Game/ENGINE.d build/asm/Game/PLAYER.d build/src/Game/VEHICLE/vehicle.d build/asm/Game/MAIN.d build/asm/LIBGPU.d build/asm/LIBSPU.d build/asm/data/80FEC.data.d build/asm/data/96154.sdata.d build/asm/data/96920.sbss.d build/asm/data/96920.bss.d
