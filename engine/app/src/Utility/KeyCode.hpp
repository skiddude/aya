#pragma once

// This code was adapted from SDL via GNU license below

/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997, 1998, 1999, 2000, 2001, 2002  Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    Sam Lantinga
    slouken@libsdl.org
*/

// THIS  MIGHT COMPLETELY MESS UP THE INPUTS IDK NOR DO I CARE RN

namespace Aya
{

    enum KeyCode
    {
        AYA_SDLK_UNKNOWN = 0,
        AYA_SDLK_BACKSPACE = 8,
        AYA_SDLK_TAB = 9,
        AYA_SDLK_CLEAR = 12,
        AYA_SDLK_RETURN = 13,
        AYA_SDLK_PAUSE = 19,
        AYA_SDLK_ESCAPE = 27,
        AYA_SDLK_SPACE = 32,
        AYA_SDLK_EXCLAIM = 33,
        AYA_SDLK_QUOTEDBL = 34,
        AYA_SDLK_HASH = 35,
        AYA_SDLK_DOLLAR = 36,
        AYA_SDLK_PERCENT = 37,
        AYA_SDLK_AMPERSAND = 38,
        AYA_SDLK_QUOTE = 39,
        AYA_SDLK_LEFTPAREN = 40,
        AYA_SDLK_RIGHTPAREN = 41,
        AYA_SDLK_ASTERISK = 42,
        AYA_SDLK_PLUS = 43,
        AYA_SDLK_COMMA = 44,
        AYA_SDLK_MINUS = 45,
        AYA_SDLK_PERIOD = 46,
        AYA_SDLK_SLASH = 47,
        AYA_SDLK_0 = 48,
        AYA_SDLK_1 = 49,
        AYA_SDLK_2 = 50,
        AYA_SDLK_3 = 51,
        AYA_SDLK_4 = 52,
        AYA_SDLK_5 = 53,
        AYA_SDLK_6 = 54,
        AYA_SDLK_7 = 55,
        AYA_SDLK_8 = 56,
        AYA_SDLK_9 = 57,
        AYA_SDLK_COLON = 58,
        AYA_SDLK_SEMICOLON = 59,
        AYA_SDLK_LESS = 60,
        AYA_SDLK_EQUALS = 61,
        AYA_SDLK_GREATER = 62,
        AYA_SDLK_QUESTION = 63,
        AYA_SDLK_AT = 64,
        /*
           Skip uppercase letters
         */
        AYA_SDLK_LEFTBRACKET = 91,
        AYA_SDLK_BACKSLASH = 92,
        AYA_SDLK_RIGHTBRACKET = 93,
        AYA_SDLK_CARET = 94,
        AYA_SDLK_UNDERSCORE = 95,
        AYA_SDLK_BACKQUOTE = 96,
        AYA_SDLK_a = 97,
        AYA_SDLK_b = 98,
        AYA_SDLK_c = 99,
        AYA_SDLK_d = 100,
        AYA_SDLK_e = 101,
        AYA_SDLK_f = 102,
        AYA_SDLK_g = 103,
        AYA_SDLK_h = 104,
        AYA_SDLK_i = 105,
        AYA_SDLK_j = 106,
        AYA_SDLK_k = 107,
        AYA_SDLK_l = 108,
        AYA_SDLK_m = 109,
        AYA_SDLK_n = 110,
        AYA_SDLK_o = 111,
        AYA_SDLK_p = 112,
        AYA_SDLK_q = 113,
        AYA_SDLK_r = 114,
        AYA_SDLK_s = 115,
        AYA_SDLK_t = 116,
        AYA_SDLK_u = 117,
        AYA_SDLK_v = 118,
        AYA_SDLK_w = 119,
        AYA_SDLK_x = 120,
        AYA_SDLK_y = 121,
        AYA_SDLK_z = 122,

        AYA_SDLK_LEFTCURLY = 123,
        AYA_SDLK_PIPE = 124,
        AYA_SDLK_RIGHTCURLY = 125,
        AYA_SDLK_TILDE = 126,
        AYA_SDLK_DELETE = 127,
        /* End of ASCII mapped keysyms */

        /* International keyboard syms */
        AYA_SDLK_WORLD_0 = 160, /* 0xA0 */
        AYA_SDLK_WORLD_1 = 161,
        AYA_SDLK_WORLD_2 = 162,
        AYA_SDLK_WORLD_3 = 163,
        AYA_SDLK_WORLD_4 = 164,
        AYA_SDLK_WORLD_5 = 165,
        AYA_SDLK_WORLD_6 = 166,
        AYA_SDLK_WORLD_7 = 167,
        AYA_SDLK_WORLD_8 = 168,
        AYA_SDLK_WORLD_9 = 169,
        AYA_SDLK_WORLD_10 = 170,
        AYA_SDLK_WORLD_11 = 171,
        AYA_SDLK_WORLD_12 = 172,
        AYA_SDLK_WORLD_13 = 173,
        AYA_SDLK_WORLD_14 = 174,
        AYA_SDLK_WORLD_15 = 175,
        AYA_SDLK_WORLD_16 = 176,
        AYA_SDLK_WORLD_17 = 177,
        AYA_SDLK_WORLD_18 = 178,
        AYA_SDLK_WORLD_19 = 179,
        AYA_SDLK_WORLD_20 = 180,
        AYA_SDLK_WORLD_21 = 181,
        AYA_SDLK_WORLD_22 = 182,
        AYA_SDLK_WORLD_23 = 183,
        AYA_SDLK_WORLD_24 = 184,
        AYA_SDLK_WORLD_25 = 185,
        AYA_SDLK_WORLD_26 = 186,
        AYA_SDLK_WORLD_27 = 187,
        AYA_SDLK_WORLD_28 = 188,
        AYA_SDLK_WORLD_29 = 189,
        AYA_SDLK_WORLD_30 = 190,
        AYA_SDLK_WORLD_31 = 191,
        AYA_SDLK_WORLD_32 = 192,
        AYA_SDLK_WORLD_33 = 193,
        AYA_SDLK_WORLD_34 = 194,
        AYA_SDLK_WORLD_35 = 195,
        AYA_SDLK_WORLD_36 = 196,
        AYA_SDLK_WORLD_37 = 197,
        AYA_SDLK_WORLD_38 = 198,
        AYA_SDLK_WORLD_39 = 199,
        AYA_SDLK_WORLD_40 = 200,
        AYA_SDLK_WORLD_41 = 201,
        AYA_SDLK_WORLD_42 = 202,
        AYA_SDLK_WORLD_43 = 203,
        AYA_SDLK_WORLD_44 = 204,
        AYA_SDLK_WORLD_45 = 205,
        AYA_SDLK_WORLD_46 = 206,
        AYA_SDLK_WORLD_47 = 207,
        AYA_SDLK_WORLD_48 = 208,
        AYA_SDLK_WORLD_49 = 209,
        AYA_SDLK_WORLD_50 = 210,
        AYA_SDLK_WORLD_51 = 211,
        AYA_SDLK_WORLD_52 = 212,
        AYA_SDLK_WORLD_53 = 213,
        AYA_SDLK_WORLD_54 = 214,
        AYA_SDLK_WORLD_55 = 215,
        AYA_SDLK_WORLD_56 = 216,
        AYA_SDLK_WORLD_57 = 217,
        AYA_SDLK_WORLD_58 = 218,
        AYA_SDLK_WORLD_59 = 219,
        AYA_SDLK_WORLD_60 = 220,
        AYA_SDLK_WORLD_61 = 221,
        AYA_SDLK_WORLD_62 = 222,
        AYA_SDLK_WORLD_63 = 223,
        AYA_SDLK_WORLD_64 = 224,
        AYA_SDLK_WORLD_65 = 225,
        AYA_SDLK_WORLD_66 = 226,
        AYA_SDLK_WORLD_67 = 227,
        AYA_SDLK_WORLD_68 = 228,
        AYA_SDLK_WORLD_69 = 229,
        AYA_SDLK_WORLD_70 = 230,
        AYA_SDLK_WORLD_71 = 231,
        AYA_SDLK_WORLD_72 = 232,
        AYA_SDLK_WORLD_73 = 233,
        AYA_SDLK_WORLD_74 = 234,
        AYA_SDLK_WORLD_75 = 235,
        AYA_SDLK_WORLD_76 = 236,
        AYA_SDLK_WORLD_77 = 237,
        AYA_SDLK_WORLD_78 = 238,
        AYA_SDLK_WORLD_79 = 239,
        AYA_SDLK_WORLD_80 = 240,
        AYA_SDLK_WORLD_81 = 241,
        AYA_SDLK_WORLD_82 = 242,
        AYA_SDLK_WORLD_83 = 243,
        AYA_SDLK_WORLD_84 = 244,
        AYA_SDLK_WORLD_85 = 245,
        AYA_SDLK_WORLD_86 = 246,
        AYA_SDLK_WORLD_87 = 247,
        AYA_SDLK_WORLD_88 = 248,
        AYA_SDLK_WORLD_89 = 249,
        AYA_SDLK_WORLD_90 = 250,
        AYA_SDLK_WORLD_91 = 251,
        AYA_SDLK_WORLD_92 = 252,
        AYA_SDLK_WORLD_93 = 253,
        AYA_SDLK_WORLD_94 = 254,
        AYA_SDLK_WORLD_95 = 255, /* 0xFF */

        /* Numeric keypad */
        AYA_SDLK_KP0 = 256,
        AYA_SDLK_KP1 = 257,
        AYA_SDLK_KP2 = 258,
        AYA_SDLK_KP3 = 259,
        AYA_SDLK_KP4 = 260,
        AYA_SDLK_KP5 = 261,
        AYA_SDLK_KP6 = 262,
        AYA_SDLK_KP7 = 263,
        AYA_SDLK_KP8 = 264,
        AYA_SDLK_KP9 = 265,
        AYA_SDLK_KP_PERIOD = 266,
        AYA_SDLK_KP_DIVIDE = 267,
        AYA_SDLK_KP_MULTIPLY = 268,
        AYA_SDLK_KP_MINUS = 269,
        AYA_SDLK_KP_PLUS = 270,
        AYA_SDLK_KP_ENTER = 271,
        AYA_SDLK_KP_EQUALS = 272,

        /* Arrows + Home/End pad */
        AYA_SDLK_UP = 273,
        AYA_SDLK_DOWN = 274,
        AYA_SDLK_RIGHT = 275,
        AYA_SDLK_LEFT = 276,
        AYA_SDLK_INSERT = 277,
        AYA_SDLK_HOME = 278,
        AYA_SDLK_END = 279,
        AYA_SDLK_PAGEUP = 280,
        AYA_SDLK_PAGEDOWN = 281,

        /* Function keys */
        AYA_SDLK_F1 = 282,
        AYA_SDLK_F2 = 283,
        AYA_SDLK_F3 = 284,
        AYA_SDLK_F4 = 285,
        AYA_SDLK_F5 = 286,
        AYA_SDLK_F6 = 287,
        AYA_SDLK_F7 = 288,
        AYA_SDLK_F8 = 289,
        AYA_SDLK_F9 = 290,
        AYA_SDLK_F10 = 291,
        AYA_SDLK_F11 = 292,
        AYA_SDLK_F12 = 293,
        AYA_SDLK_F13 = 294,
        AYA_SDLK_F14 = 295,
        AYA_SDLK_F15 = 296,

        /* Key state modifier keys */
        AYA_SDLK_NUMLOCK = 300,
        AYA_SDLK_CAPSLOCK = 301,
        AYA_SDLK_SCROLLOCK = 302,
        AYA_SDLK_RSHIFT = 303,
        AYA_SDLK_LSHIFT = 304,
        AYA_SDLK_RCTRL = 305,
        AYA_SDLK_LCTRL = 306,
        AYA_SDLK_RALT = 307,
        AYA_SDLK_LALT = 308,
        AYA_SDLK_RMETA = 309,
        AYA_SDLK_LMETA = 310,
        AYA_SDLK_LSUPER = 311,  /* Left "Windows" key */
        AYA_SDLK_RSUPER = 312,  /* Right "Windows" key */
        AYA_SDLK_MODE = 313,    /* "Alt Gr" key */
        AYA_SDLK_COMPOSE = 314, /* Multi-key compose key */

        /* Miscellaneous function keys */
        AYA_SDLK_HELP = 315,
        AYA_SDLK_PRINT = 316,
        AYA_SDLK_SYSREQ = 317,
        AYA_SDLK_BREAK = 318,
        AYA_SDLK_MENU = 319,
        AYA_SDLK_POWER = 320, /* Power Macintosh power key */
        AYA_SDLK_EURO = 321,  /* Some european keyboards */
        AYA_SDLK_UNDO = 322,  /* Atari keyboard has Undo */

        /* Add any other keys here */

        // Aya Gamepad stuff
        AYA_SDLK_GAMEPAD_BUTTONX = 1000,
        AYA_SDLK_GAMEPAD_BUTTONY = 1001,
        AYA_SDLK_GAMEPAD_BUTTONA = 1002,
        AYA_SDLK_GAMEPAD_BUTTONB = 1003,
        AYA_SDLK_GAMEPAD_BUTTONR1 = 1004,
        AYA_SDLK_GAMEPAD_BUTTONL1 = 1005,
        AYA_SDLK_GAMEPAD_BUTTONR2 = 1006,
        AYA_SDLK_GAMEPAD_BUTTONL2 = 1007,
        AYA_SDLK_GAMEPAD_BUTTONR3 = 1008,
        AYA_SDLK_GAMEPAD_BUTTONL3 = 1009,
        AYA_SDLK_GAMEPAD_BUTTONSTART = 1010,
        AYA_SDLK_GAMEPAD_BUTTONSELECT = 1011,
        AYA_SDLK_GAMEPAD_DPADLEFT = 1012,
        AYA_SDLK_GAMEPAD_DPADRIGHT = 1013,
        AYA_SDLK_GAMEPAD_DPADUP = 1014,
        AYA_SDLK_GAMEPAD_DPADDOWN = 1015,
        AYA_SDLK_GAMEPAD_THUMBSTICK1 = 1016,
        AYA_SDLK_GAMEPAD_THUMBSTICK2 = 1017,

        AYA_SDLK_LAST
    };
    enum ModCode
    {
        AYA_KMOD_NONE = 0x0000,
        AYA_KMOD_LSHIFT = 0x0001,
        AYA_KMOD_RSHIFT = 0x0002,
        AYA_KMOD_LCTRL = 0x0040,
        AYA_KMOD_RCTRL = 0x0080,
        AYA_KMOD_LALT = 0x0100,
        AYA_KMOD_RALT = 0x0200,
        AYA_KMOD_LMETA = 0x0400,
        AYA_KMOD_RMETA = 0x0800,
        AYA_KMOD_NUM = 0x1000,
        AYA_KMOD_CAPS = 0x2000,
        AYA_KMOD_MODE = 0x4000,
        AYA_KMOD_RESERVED = 0x8000
    };

} // namespace Aya