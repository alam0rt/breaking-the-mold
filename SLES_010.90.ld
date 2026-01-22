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
        build/asm/core/early.o(.text);
        main_RODATA_START = .;
        build/asm/data/E2C.rodata.o(.rodata);
        build/asm/data/1F28.rodata.o(.rodata);
        build/asm/data/2044.rodata.o(.rodata);
        build/asm/data/LIBS_1.rodata.o(.rodata);
        build/asm/data/LIBS_2.rodata.o(.rodata);
        build/asm/data/LIBS_3.rodata.o(.rodata);
        build/asm/core/graphics.o(.text);
        build/asm/entity/core.o(.text);
        build/asm/entity/behaviors.o(.text);
        build/asm/player/main.o(.text);
        build/asm/level/loader.o(.text);
        build/asm/LIBS.o(.text);
        main_TEXT_END = .;
        main_TEXT_SIZE = ABSOLUTE(main_TEXT_END - main_TEXT_START);
        main_DATA_START = .;
        build/asm/core/early.o(.data);
        build/asm/core/graphics.o(.data);
        build/asm/entity/core.o(.data);
        build/asm/entity/behaviors.o(.data);
        build/asm/player/main.o(.data);
        build/asm/level/loader.o(.data);
        build/asm/LIBS.o(.data);
        build/asm/data/80FEC.data.o(.data);
        main_DATA_END = .;
        main_DATA_SIZE = ABSOLUTE(main_DATA_END - main_DATA_START);
        build/asm/core/early.o(.rodata);
        build/asm/core/graphics.o(.rodata);
        build/asm/entity/core.o(.rodata);
        build/asm/entity/behaviors.o(.rodata);
        build/asm/player/main.o(.rodata);
        build/asm/level/loader.o(.rodata);
        build/asm/LIBS.o(.rodata);
        main_RODATA_END = .;
        main_RODATA_SIZE = ABSOLUTE(main_RODATA_END - main_RODATA_START);
        main_SDATA_START = .;
        build/asm/core/early.o(.sdata);
        build/asm/core/graphics.o(.sdata);
        build/asm/entity/core.o(.sdata);
        build/asm/entity/behaviors.o(.sdata);
        build/asm/player/main.o(.sdata);
        build/asm/level/loader.o(.sdata);
        build/asm/LIBS.o(.sdata);
        build/asm/data/96154.sdata.o(.sdata);
        main_SDATA_END = .;
        main_SDATA_SIZE = ABSOLUTE(main_SDATA_END - main_SDATA_START);
        . += 0x6E0;
        main_BSS_START = .;
        build/asm/core/early.o(.bss);
        build/asm/core/graphics.o(.bss);
        build/asm/entity/core.o(.bss);
        build/asm/entity/behaviors.o(.bss);
        build/asm/player/main.o(.bss);
        build/asm/level/loader.o(.bss);
        build/asm/LIBS.o(.bss);
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
