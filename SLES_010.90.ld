SECTIONS
{
    __romPos = 0;
    _gp = 0x800A5954;
    header_ROM_START = __romPos;
    header_VRAM = ADDR(.header);
    .header : AT(header_ROM_START) SUBALIGN(2)
    {
        FILL(0x00000000);
        header_DATA_START = .;
        build/asm/header.o(.data);
        header_DATA_END = .;
        header_DATA_SIZE = ABSOLUTE(header_DATA_END - header_DATA_START);
    }
    __romPos += SIZEOF(.header);
    header_ROM_END = __romPos;
    header_VRAM_END = .;

    main_ROM_START = __romPos;
    main_VRAM = ADDR(.main);
    .main 0x80010000 : AT(main_ROM_START) SUBALIGN(2)
    {
        FILL(0x00000000);
        main_TEXT_START = .;
        build/src/Game/EARLY/bootstrap.o(.text);
        main_RODATA_START = .;
        build/asm/data/E2C.rodata.o(.rodata);
        build/asm/data/1F28.rodata.o(.rodata);
        build/asm/data/2044.rodata.o(.rodata);
        build/asm/data/LIBCD.rodata.o(.rodata);
        build/asm/data/LIBGPU.rodata.o(.rodata);
        build/asm/data/LIBSPU.rodata.o(.rodata);
        build/src/Game/RENDER/GetFrameReadyFlag.o(.rodata);
        build/asm/Game/RENDER.o(.rodata);
        build/asm/Game/ENTITY.o(.rodata);
        build/asm/Game/OBJECT.o(.rodata);
        build/asm/Game/PLAYER.o(.rodata);
        build/asm/Game/GAMELOOP.o(.rodata);
        build/asm/LIBS.o(.rodata);
        main_SDATA_START = .;
        build/src/Game/EARLY/bootstrap.o(.sdata);
        build/asm/data/E2C.rodata.o(.sdata);
        build/asm/data/1F28.rodata.o(.sdata);
        build/asm/data/2044.rodata.o(.sdata);
        build/asm/data/LIBCD.rodata.o(.sdata);
        build/asm/data/LIBGPU.rodata.o(.sdata);
        build/asm/data/LIBSPU.rodata.o(.sdata);
        build/src/Game/RENDER/GetFrameReadyFlag.o(.sdata);
        build/asm/Game/RENDER.o(.sdata);
        build/asm/Game/ENTITY.o(.sdata);
        build/asm/Game/OBJECT.o(.sdata);
        build/asm/Game/PLAYER.o(.sdata);
        build/asm/Game/GAMELOOP.o(.sdata);
        build/asm/LIBS.o(.sdata);
        main_SBSS_START = .;
        build/src/Game/EARLY/bootstrap.o(.sbss);
        build/asm/data/E2C.rodata.o(.sbss);
        build/asm/data/1F28.rodata.o(.sbss);
        build/asm/data/2044.rodata.o(.sbss);
        build/asm/data/LIBCD.rodata.o(.sbss);
        build/asm/data/LIBGPU.rodata.o(.sbss);
        build/asm/data/LIBSPU.rodata.o(.sbss);
        build/src/Game/RENDER/GetFrameReadyFlag.o(.sbss);
        build/asm/Game/RENDER.o(.sbss);
        build/asm/Game/ENTITY.o(.sbss);
        build/asm/Game/OBJECT.o(.sbss);
        build/asm/Game/PLAYER.o(.sbss);
        build/asm/Game/GAMELOOP.o(.sbss);
        build/asm/LIBS.o(.sbss);
        main_BSS_START = .;
        build/src/Game/EARLY/bootstrap.o(.bss);
        build/asm/data/E2C.rodata.o(.bss);
        build/asm/data/1F28.rodata.o(.bss);
        build/asm/data/2044.rodata.o(.bss);
        build/asm/data/LIBCD.rodata.o(.bss);
        build/asm/data/LIBGPU.rodata.o(.bss);
        build/asm/data/LIBSPU.rodata.o(.bss);
        build/src/Game/RENDER/GetFrameReadyFlag.o(.bss);
        build/asm/Game/RENDER.o(.bss);
        build/asm/Game/ENTITY.o(.bss);
        build/asm/Game/OBJECT.o(.bss);
        build/asm/Game/PLAYER.o(.bss);
        build/asm/Game/GAMELOOP.o(.bss);
        build/asm/LIBS.o(.bss);
        build/src/Game/RENDER/GetFrameReadyFlag.o(.text);
        build/asm/Game/RENDER.o(.text);
        build/asm/Game/ENTITY.o(.text);
        build/asm/Game/OBJECT.o(.text);
        build/asm/Game/PLAYER.o(.text);
        build/asm/Game/GAMELOOP.o(.text);
        build/asm/LIBS.o(.text);
        main_TEXT_END = .;
        main_TEXT_SIZE = ABSOLUTE(main_TEXT_END - main_TEXT_START);
        main_DATA_START = .;
        build/src/Game/EARLY/bootstrap.o(.data);
        build/asm/data/E2C.rodata.o(.data);
        build/asm/data/1F28.rodata.o(.data);
        build/asm/data/2044.rodata.o(.data);
        build/asm/data/LIBCD.rodata.o(.data);
        build/asm/data/LIBGPU.rodata.o(.data);
        build/asm/data/LIBSPU.rodata.o(.data);
        build/src/Game/RENDER/GetFrameReadyFlag.o(.data);
        build/asm/Game/RENDER.o(.data);
        build/asm/Game/ENTITY.o(.data);
        build/asm/Game/OBJECT.o(.data);
        build/asm/Game/PLAYER.o(.data);
        build/asm/Game/GAMELOOP.o(.data);
        build/asm/LIBS.o(.data);
        build/asm/data/80FEC.data.o(.data);
        main_DATA_END = .;
        main_DATA_SIZE = ABSOLUTE(main_DATA_END - main_DATA_START);
        build/src/Game/EARLY/bootstrap.o(.rodata);
        main_RODATA_END = .;
        main_RODATA_SIZE = ABSOLUTE(main_RODATA_END - main_RODATA_START);
        build/asm/data/96154.sdata.o(.sdata);
        main_SDATA_END = .;
        main_SDATA_SIZE = ABSOLUTE(main_SDATA_END - main_SDATA_START);
        build/asm/data/96920.sbss.o(.sbss);
        main_SBSS_END = .;
        main_SBSS_SIZE = ABSOLUTE(main_SBSS_END - main_SBSS_START);
        build/asm/data/96920.bss.o(.bss);
        main_BSS_END = .;
        main_BSS_SIZE = ABSOLUTE(main_BSS_END - main_BSS_START);
    }
    __romPos += SIZEOF(.main);
    __romPos = ALIGN(__romPos, 16);
    main_ROM_END = __romPos;
    main_VRAM_END = .;

    /DISCARD/ :
    {
        *(*);
    }
}
