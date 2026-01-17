#ifndef GAME_STATE_H
#define GAME_STATE_H

#include "common.h"
#include "Game/entity.h"

/* -----------------------------------------------------------------------------
 * GameState
 * Central game state structure at 0x8009DC40 (PAL). Initialized by InitGameState @ 0x80020568. Contains entity lists, camera, player refs, and level context.
 * Size: 386 bytes (0x182)
 * Exported from Ghidra
 * ----------------------------------------------------------------------------- */

typedef struct {
    /* 0x00 */ u16      mode_base_offset;  // Base offset for callback parameter
    /* 0x02 */ u16      mode_callback_index;  // Callback table index (-1 = direct u32)
    /* 0x04 */ u32  mode_callback_ptr;  // Callback u32 or table base
    /* 0x08 */ u32  unknown08;  // Unknown u32
    /* 0x0C */ u32  level_data_context_ptr;  // u32 to LevelDataContext at +0x84
    /* 0x10 */ u8       field_10;
    /* 0x11 */ u8       field_11;
    /* 0x12 */ u8       field_12;
    /* 0x13 */ u8       field_13;
    /* 0x14 */ u8       field_14;
    /* 0x15 */ u8       field_15;
    /* 0x16 */ u8       field_16;
    /* 0x17 */ u8       field_17;
    /* 0x18 */ u8       field_18;
    /* 0x19 */ u8       field_19;
    /* 0x1A */ u8       field_1A;
    /* 0x1B */ u8       field_1B;
    /* 0x1C */ EntityListNode  *entity_tick_list_head;  // Head of tick-sorted entity list (iterated by EntityTickLoop)
    /* 0x20 */ EntityListNode  *entity_render_list_head;  // Head of render-sorted entity list (z-order)
    /* 0x24 */ EntityListNode  *entity_collision_list_head;  // Entity collision/update queue head
    /* 0x28 */ u32  entity_pool;  // Raw entity definition pool from Asset 501
    /* 0x2C */ u32  player_alt;  // Alternate player entity reference
    /* 0x30 */ u32  player_entity_ptr;  // Main player entity u32
    /* 0x34 */ u8       field_34;
    /* 0x35 */ u8       field_35;
    /* 0x36 */ u8       field_36;
    /* 0x37 */ u8       field_37;
    /* 0x38 */ u16      camera_x;  // Camera X position (pixels)
    /* 0x3A */ u16      camera_x_high;  // Camera X high word
    /* 0x3C */ u16      camera_y;  // Camera Y position (pixels)
    /* 0x3E */ u16      camera_y_high;  // Camera Y high word
    /* 0x40 */ u8       field_40;
    /* 0x41 */ u8       field_41;
    /* 0x42 */ u8       field_42;
    /* 0x43 */ u8       field_43;
    /* 0x44 */ u8       field_44;
    /* 0x45 */ u8       field_45;
    /* 0x46 */ u8       field_46;
    /* 0x47 */ u8       field_47;
    /* 0x48 */ u8       field_48;
    /* 0x49 */ u8       field_49;
    /* 0x4A */ u8       field_4A;
    /* 0x4B */ u8       field_4B;
    /* 0x4C */ u8       field_4C;
    /* 0x4D */ u8       field_4D;
    /* 0x4E */ u8       field_4E;
    /* 0x4F */ u8       field_4F;
    /* 0x50 */ u32  input_state_ptr;  // Input state u32 (g_pPlayer1Input)
    /* 0x54 */ u8       field_54;
    /* 0x55 */ u8       field_55;
    /* 0x56 */ u8       field_56;
    /* 0x57 */ u8       field_57;
    /* 0x58 */ u8       field_58;
    /* 0x59 */ u8       field_59;
    /* 0x5A */ u8       field_5A;
    /* 0x5B */ u8       field_5B;
    /* 0x5C */ u8       field_5C;
    /* 0x5D */ u8       field_5D;
    /* 0x5E */ u8       field_5E;
    /* 0x5F */ u8       field_5F;
    /* 0x60 */ u8       field_60;
    /* 0x61 */ u8       field_61;
    /* 0x62 */ u8       field_62;
    /* 0x63 */ u8       field_63;
    /* 0x64 */ u8       field_64;
    /* 0x65 */ u8       field_65;
    /* 0x66 */ u8       field_66;
    /* 0x67 */ u8       field_67;
    /* 0x68 */ u8       field_68;
    /* 0x69 */ u8       field_69;
    /* 0x6A */ u8       field_6A;
    /* 0x6B */ u8       field_6B;
    /* 0x6C */ u8       field_6C;
    /* 0x6D */ u8       field_6D;
    /* 0x6E */ u8       field_6E;
    /* 0x6F */ u8       field_6F;
    /* 0x70 */ u8       field_70;
    /* 0x71 */ u8       field_71;
    /* 0x72 */ u8       field_72;
    /* 0x73 */ u8       field_73;
    /* 0x74 */ u8       field_74;
    /* 0x75 */ u8       field_75;
    /* 0x76 */ u8       field_76;
    /* 0x77 */ u8       field_77;
    /* 0x78 */ u8       field_78;
    /* 0x79 */ u8       field_79;
    /* 0x7A */ u8       field_7A;
    /* 0x7B */ u8       field_7B;
    /* 0x7C */ u32  callback_table_ptr;  // Entity type callback table u32 (PAL: 0x8009D5F8)
    /* 0x80 */ u8       field_80;
    /* 0x81 */ u8       field_81;
    /* 0x82 */ u8       field_82;
    /* 0x83 */ u8       field_83;
    /* 0x84 */ u8       level_data_context;  // LevelDataContext embedded structure (128 bytes)
    /* 0x85 */ u8       field_85;
    /* 0x86 */ u8       field_86;
    /* 0x87 */ u8       field_87;
    /* 0x88 */ u8       field_88;
    /* 0x89 */ u8       field_89;
    /* 0x8A */ u8       field_8A;
    /* 0x8B */ u8       field_8B;
    /* 0x8C */ u8       field_8C;
    /* 0x8D */ u8       field_8D;
    /* 0x8E */ u8       field_8E;
    /* 0x8F */ u8       field_8F;
    /* 0x90 */ u8       field_90;
    /* 0x91 */ u8       field_91;
    /* 0x92 */ u8       field_92;
    /* 0x93 */ u8       field_93;
    /* 0x94 */ u8       field_94;
    /* 0x95 */ u8       field_95;
    /* 0x96 */ u8       field_96;
    /* 0x97 */ u8       field_97;
    /* 0x98 */ u8       field_98;
    /* 0x99 */ u8       field_99;
    /* 0x9A */ u8       field_9A;
    /* 0x9B */ u8       field_9B;
    /* 0x9C */ u8       field_9C;
    /* 0x9D */ u8       field_9D;
    /* 0x9E */ u8       field_9E;
    /* 0x9F */ u8       field_9F;
    /* 0xA0 */ u8       field_A0;
    /* 0xA1 */ u8       field_A1;
    /* 0xA2 */ u8       field_A2;
    /* 0xA3 */ u8       field_A3;
    /* 0xA4 */ u8       field_A4;
    /* 0xA5 */ u8       field_A5;
    /* 0xA6 */ u8       field_A6;
    /* 0xA7 */ u8       field_A7;
    /* 0xA8 */ u8       field_A8;
    /* 0xA9 */ u8       field_A9;
    /* 0xAA */ u8       field_AA;
    /* 0xAB */ u8       field_AB;
    /* 0xAC */ u8       field_AC;
    /* 0xAD */ u8       field_AD;
    /* 0xAE */ u8       field_AE;
    /* 0xAF */ u8       field_AF;
    /* 0xB0 */ u8       field_B0;
    /* 0xB1 */ u8       field_B1;
    /* 0xB2 */ u8       field_B2;
    /* 0xB3 */ u8       field_B3;
    /* 0xB4 */ u8       field_B4;
    /* 0xB5 */ u8       field_B5;
    /* 0xB6 */ u8       field_B6;
    /* 0xB7 */ u8       field_B7;
    /* 0xB8 */ u8       field_B8;
    /* 0xB9 */ u8       field_B9;
    /* 0xBA */ u8       field_BA;
    /* 0xBB */ u8       field_BB;
    /* 0xBC */ u8       field_BC;
    /* 0xBD */ u8       field_BD;
    /* 0xBE */ u8       field_BE;
    /* 0xBF */ u8       field_BF;
    /* 0xC0 */ u8       field_C0;
    /* 0xC1 */ u8       field_C1;
    /* 0xC2 */ u8       field_C2;
    /* 0xC3 */ u8       field_C3;
    /* 0xC4 */ u8       field_C4;
    /* 0xC5 */ u8       field_C5;
    /* 0xC6 */ u8       field_C6;
    /* 0xC7 */ u8       field_C7;
    /* 0xC8 */ u8       field_C8;
    /* 0xC9 */ u8       field_C9;
    /* 0xCA */ u8       field_CA;
    /* 0xCB */ u8       field_CB;
    /* 0xCC */ u8       field_CC;
    /* 0xCD */ u8       field_CD;
    /* 0xCE */ u8       field_CE;
    /* 0xCF */ u8       field_CF;
    /* 0xD0 */ u8       field_D0;
    /* 0xD1 */ u8       field_D1;
    /* 0xD2 */ u8       field_D2;
    /* 0xD3 */ u8       field_D3;
    /* 0xD4 */ u8       field_D4;
    /* 0xD5 */ u8       field_D5;
    /* 0xD6 */ u8       field_D6;
    /* 0xD7 */ u8       field_D7;
    /* 0xD8 */ u8       field_D8;
    /* 0xD9 */ u8       field_D9;
    /* 0xDA */ u8       field_DA;
    /* 0xDB */ u8       field_DB;
    /* 0xDC */ u8       field_DC;
    /* 0xDD */ u8       field_DD;
    /* 0xDE */ u8       field_DE;
    /* 0xDF */ u8       field_DF;
    /* 0xE0 */ u8       field_E0;
    /* 0xE1 */ u8       field_E1;
    /* 0xE2 */ u8       field_E2;
    /* 0xE3 */ u8       field_E3;
    /* 0xE4 */ u8       field_E4;
    /* 0xE5 */ u8       field_E5;
    /* 0xE6 */ u8       field_E6;
    /* 0xE7 */ u8       field_E7;
    /* 0xE8 */ u8       field_E8;
    /* 0xE9 */ u8       field_E9;
    /* 0xEA */ u8       field_EA;
    /* 0xEB */ u8       field_EB;
    /* 0xEC */ u8       field_EC;
    /* 0xED */ u8       field_ED;
    /* 0xEE */ u8       field_EE;
    /* 0xEF */ u8       field_EF;
    /* 0xF0 */ u8       field_F0;
    /* 0xF1 */ u8       field_F1;
    /* 0xF2 */ u8       field_F2;
    /* 0xF3 */ u8       field_F3;
    /* 0xF4 */ u8       field_F4;
    /* 0xF5 */ u8       field_F5;
    /* 0xF6 */ u8       field_F6;
    /* 0xF7 */ u8       field_F7;
    /* 0xF8 */ u8       field_F8;
    /* 0xF9 */ u8       field_F9;
    /* 0xFA */ u8       field_FA;
    /* 0xFB */ u8       field_FB;
    /* 0xFC */ u8       field_FC;
    /* 0xFD */ u8       field_FD;
    /* 0xFE */ u8       field_FE;
    /* 0xFF */ u8       field_FF;
    /* 0x100 */ u8       field_100;
    /* 0x101 */ u8       field_101;
    /* 0x102 */ u8       field_102;
    /* 0x103 */ u8       field_103;
    /* 0x104 */ u8       field_104;
    /* 0x105 */ u8       field_105;
    /* 0x106 */ u8       field_106;
    /* 0x107 */ u8       field_107;
    /* 0x108 */ u8       field_108;
    /* 0x109 */ u8       field_109;
    /* 0x10A */ u8       field_10A;
    /* 0x10B */ u8       field_10B;
    /* 0x10C */ u32      frame_parity;  // Frame parity/alternation value, modulo'd for path offsets
    /* 0x110 */ u8       field_110;
    /* 0x111 */ u8       field_111;
    /* 0x112 */ u8       field_112;
    /* 0x113 */ u8       field_113;
    /* 0x114 */ u8       field_114;
    /* 0x115 */ u8       field_115;
    /* 0x116 */ u8       field_116;
    /* 0x117 */ u8       field_117;
    /* 0x118 */ u8       field_118;
    /* 0x119 */ u8       field_119;
    /* 0x11A */ u8       field_11A;
    /* 0x11B */ u8       field_11B;
    /* 0x11C */ u8       field_11C;
    /* 0x11D */ u8       field_11D;
    /* 0x11E */ u8       field_11E;
    /* 0x11F */ u8       field_11F;
    /* 0x120 */ u32      player_scale;  // Player scale factor (0x8000-0x10000)
    /* 0x124 */ u8       field_124;
    /* 0x125 */ u8       field_125;
    /* 0x126 */ u8       field_126;
    /* 0x127 */ u8       field_127;
    /* 0x128 */ u8       player_tint_r;  // Player RGB tint (red)
    /* 0x129 */ u8       player_tint_g;  // Player RGB tint (green)
    /* 0x12A */ u8       player_tint_b;  // Player RGB tint (blue)
    /* 0x12B */ u8       field_12B;
    /* 0x12C */ u8       field_12C;
    /* 0x12D */ u8       field_12D;
    /* 0x12E */ u8       field_12E;
    /* 0x12F */ u8       field_12F;
    /* 0x130 */ u8       field_130;
    /* 0x131 */ u8       field_131;
    /* 0x132 */ u8       field_132;
    /* 0x133 */ u8       field_133;
    /* 0x134 */ u8       bg_color_change_flag;  // Background color change request flag (cleared by RenderEntities)
    /* 0x135 */ u8       bg_color_r;  // Background color (red)
    /* 0x136 */ u8       bg_color_g;  // Background color (green)
    /* 0x137 */ u8       bg_color_b;  // Background color (blue)
    /* 0x138 */ u32  checkpoint_entity_list;  // Saved entity list for checkpoint respawn
    /* 0x13C */ u8       field_13C;
    /* 0x13D */ u8       field_13D;
    /* 0x13E */ u8       field_13E;
    /* 0x13F */ u8       field_13F;
    /* 0x140 */ u8       field_140;
    /* 0x141 */ u8       field_141;
    /* 0x142 */ u8       field_142;
    /* 0x143 */ u8       field_143;
    /* 0x144 */ u8       field_144;
    /* 0x145 */ u8       field_145;
    /* 0x146 */ u8       field_146;
    /* 0x147 */ u8       field_147;
    /* 0x148 */ u8       field_148;
    /* 0x149 */ u8       field_149;
    /* 0x14A */ u8       field_14A;
    /* 0x14B */ u8       field_14B;
    /* 0x14C */ u8       field_14C;
    /* 0x14D */ u8       field_14D;
    /* 0x14E */ u8       field_14E;
    /* 0x14F */ u8       field_14F;
    /* 0x150 */ u8       field_150;
    /* 0x151 */ u8       field_151;
    /* 0x152 */ u8       field_152;
    /* 0x153 */ u8       field_153;
    /* 0x154 */ u8       field_154;
    /* 0x155 */ u8       field_155;
    /* 0x156 */ u8       field_156;
    /* 0x157 */ u8       field_157;
    /* 0x158 */ u8       field_158;
    /* 0x159 */ u8       field_159;
    /* 0x15A */ u8       field_15A;
    /* 0x15B */ u8       field_15B;
    /* 0x15C */ u8       field_15C;
    /* 0x15D */ u8       field_15D;
    /* 0x15E */ u8       field_15E;
    /* 0x15F */ u8       field_15F;
    /* 0x160 */ u8       field_160;
    /* 0x161 */ u8       field_161;
    /* 0x162 */ u8       field_162;
    /* 0x163 */ u8       field_163;
    /* 0x164 */ u8       field_164;
    /* 0x165 */ u8       respawn_flag;  // Respawn requested flag
    /* 0x166 */ u8       field_166;
    /* 0x167 */ u8       field_167;
    /* 0x168 */ u8       field_168;
    /* 0x169 */ u8       field_169;
    /* 0x16A */ u8       field_16A;
    /* 0x16B */ u8       field_16B;
    /* 0x16C */ u8       field_16C;
    /* 0x16D */ u8       field_16D;
    /* 0x16E */ u8       field_16E;
    /* 0x16F */ u8       field_16F;
    /* 0x170 */ u8       field_170;
    /* 0x171 */ u8       field_171;
    /* 0x172 */ u8       field_172;
    /* 0x173 */ u8       field_173;
    /* 0x174 */ u8       field_174;
    /* 0x175 */ u8       password_level_0;  // Password-selectable level list (10 levels max)
    /* 0x176 */ u8       password_level_1;  // Password level 1
    /* 0x177 */ u8       password_level_2;  // Password level 2
    /* 0x178 */ u8       password_level_3;  // Password level 3
    /* 0x179 */ u8       password_level_4;  // Password level 4
    /* 0x17A */ u8       password_level_5;  // Password level 5
    /* 0x17B */ u8       password_level_6;  // Password level 6
    /* 0x17C */ u8       password_level_7;  // Password level 7
    /* 0x17D */ u8       password_level_8;  // Password level 8
    /* 0x17E */ u8       password_level_9;  // Password level 9
    /* 0x17F */ u8       password_level_count;  // Count of password levels (max 10)
    /* 0x180 */ u16      checkpoint_save_data;  // Checkpoint/save data (16 bytes, 8 words)
} GameState;  // Size: 0x182 (386 bytes)

#endif /* GAME_STATE_H */
