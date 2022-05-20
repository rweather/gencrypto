/*
 * Copyright (C) 2022 Southern Storm Software, Pty Ltd.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "avr/code.h"
#include "common/registry.h"
#include <cstring>

using namespace AVR;

// AES S-box (https://en.wikipedia.org/wiki/Rijndael_S-box)
static unsigned char const sbox[256] = {
    0x63, 0x7C, 0x77, 0x7B, 0xF2, 0x6B, 0x6F, 0xC5,     /* 0x00 */
    0x30, 0x01, 0x67, 0x2B, 0xFE, 0xD7, 0xAB, 0x76,
    0xCA, 0x82, 0xC9, 0x7D, 0xFA, 0x59, 0x47, 0xF0,     /* 0x10 */
    0xAD, 0xD4, 0xA2, 0xAF, 0x9C, 0xA4, 0x72, 0xC0,
    0xB7, 0xFD, 0x93, 0x26, 0x36, 0x3F, 0xF7, 0xCC,     /* 0x20 */
    0x34, 0xA5, 0xE5, 0xF1, 0x71, 0xD8, 0x31, 0x15,
    0x04, 0xC7, 0x23, 0xC3, 0x18, 0x96, 0x05, 0x9A,     /* 0x30 */
    0x07, 0x12, 0x80, 0xE2, 0xEB, 0x27, 0xB2, 0x75,
    0x09, 0x83, 0x2C, 0x1A, 0x1B, 0x6E, 0x5A, 0xA0,     /* 0x40 */
    0x52, 0x3B, 0xD6, 0xB3, 0x29, 0xE3, 0x2F, 0x84,
    0x53, 0xD1, 0x00, 0xED, 0x20, 0xFC, 0xB1, 0x5B,     /* 0x50 */
    0x6A, 0xCB, 0xBE, 0x39, 0x4A, 0x4C, 0x58, 0xCF,
    0xD0, 0xEF, 0xAA, 0xFB, 0x43, 0x4D, 0x33, 0x85,     /* 0x60 */
    0x45, 0xF9, 0x02, 0x7F, 0x50, 0x3C, 0x9F, 0xA8,
    0x51, 0xA3, 0x40, 0x8F, 0x92, 0x9D, 0x38, 0xF5,     /* 0x70 */
    0xBC, 0xB6, 0xDA, 0x21, 0x10, 0xFF, 0xF3, 0xD2,
    0xCD, 0x0C, 0x13, 0xEC, 0x5F, 0x97, 0x44, 0x17,     /* 0x80 */
    0xC4, 0xA7, 0x7E, 0x3D, 0x64, 0x5D, 0x19, 0x73,
    0x60, 0x81, 0x4F, 0xDC, 0x22, 0x2A, 0x90, 0x88,     /* 0x90 */
    0x46, 0xEE, 0xB8, 0x14, 0xDE, 0x5E, 0x0B, 0xDB,
    0xE0, 0x32, 0x3A, 0x0A, 0x49, 0x06, 0x24, 0x5C,     /* 0xA0 */
    0xC2, 0xD3, 0xAC, 0x62, 0x91, 0x95, 0xE4, 0x79,
    0xE7, 0xC8, 0x37, 0x6D, 0x8D, 0xD5, 0x4E, 0xA9,     /* 0xB0 */
    0x6C, 0x56, 0xF4, 0xEA, 0x65, 0x7A, 0xAE, 0x08,
    0xBA, 0x78, 0x25, 0x2E, 0x1C, 0xA6, 0xB4, 0xC6,     /* 0xC0 */
    0xE8, 0xDD, 0x74, 0x1F, 0x4B, 0xBD, 0x8B, 0x8A,
    0x70, 0x3E, 0xB5, 0x66, 0x48, 0x03, 0xF6, 0x0E,     /* 0xD0 */
    0x61, 0x35, 0x57, 0xB9, 0x86, 0xC1, 0x1D, 0x9E,
    0xE1, 0xF8, 0x98, 0x11, 0x69, 0xD9, 0x8E, 0x94,     /* 0xE0 */
    0x9B, 0x1E, 0x87, 0xE9, 0xCE, 0x55, 0x28, 0xDF,
    0x8C, 0xA1, 0x89, 0x0D, 0xBF, 0xE6, 0x42, 0x68,     /* 0xF0 */
    0x41, 0x99, 0x2D, 0x0F, 0xB0, 0x54, 0xBB, 0x16
};

// AES inverse S-box (https://en.wikipedia.org/wiki/Rijndael_S-box)
static unsigned char const inv_sbox[256] = {
    0x52, 0x09, 0x6A, 0xD5, 0x30, 0x36, 0xA5, 0x38,     // 0x00
    0xBF, 0x40, 0xA3, 0x9E, 0x81, 0xF3, 0xD7, 0xFB,
    0x7C, 0xE3, 0x39, 0x82, 0x9B, 0x2F, 0xFF, 0x87,     // 0x10
    0x34, 0x8E, 0x43, 0x44, 0xC4, 0xDE, 0xE9, 0xCB,
    0x54, 0x7B, 0x94, 0x32, 0xA6, 0xC2, 0x23, 0x3D,     // 0x20
    0xEE, 0x4C, 0x95, 0x0B, 0x42, 0xFA, 0xC3, 0x4E,
    0x08, 0x2E, 0xA1, 0x66, 0x28, 0xD9, 0x24, 0xB2,     // 0x30
    0x76, 0x5B, 0xA2, 0x49, 0x6D, 0x8B, 0xD1, 0x25,
    0x72, 0xF8, 0xF6, 0x64, 0x86, 0x68, 0x98, 0x16,     // 0x40
    0xD4, 0xA4, 0x5C, 0xCC, 0x5D, 0x65, 0xB6, 0x92,
    0x6C, 0x70, 0x48, 0x50, 0xFD, 0xED, 0xB9, 0xDA,     // 0x50
    0x5E, 0x15, 0x46, 0x57, 0xA7, 0x8D, 0x9D, 0x84,
    0x90, 0xD8, 0xAB, 0x00, 0x8C, 0xBC, 0xD3, 0x0A,     // 0x60
    0xF7, 0xE4, 0x58, 0x05, 0xB8, 0xB3, 0x45, 0x06,
    0xD0, 0x2C, 0x1E, 0x8F, 0xCA, 0x3F, 0x0F, 0x02,     // 0x70
    0xC1, 0xAF, 0xBD, 0x03, 0x01, 0x13, 0x8A, 0x6B,
    0x3A, 0x91, 0x11, 0x41, 0x4F, 0x67, 0xDC, 0xEA,     // 0x80
    0x97, 0xF2, 0xCF, 0xCE, 0xF0, 0xB4, 0xE6, 0x73,
    0x96, 0xAC, 0x74, 0x22, 0xE7, 0xAD, 0x35, 0x85,     // 0x90
    0xE2, 0xF9, 0x37, 0xE8, 0x1C, 0x75, 0xDF, 0x6E,
    0x47, 0xF1, 0x1A, 0x71, 0x1D, 0x29, 0xC5, 0x89,     // 0xA0
    0x6F, 0xB7, 0x62, 0x0E, 0xAA, 0x18, 0xBE, 0x1B,
    0xFC, 0x56, 0x3E, 0x4B, 0xC6, 0xD2, 0x79, 0x20,     // 0xB0
    0x9A, 0xDB, 0xC0, 0xFE, 0x78, 0xCD, 0x5A, 0xF4,
    0x1F, 0xDD, 0xA8, 0x33, 0x88, 0x07, 0xC7, 0x31,     // 0xC0
    0xB1, 0x12, 0x10, 0x59, 0x27, 0x80, 0xEC, 0x5F,
    0x60, 0x51, 0x7F, 0xA9, 0x19, 0xB5, 0x4A, 0x0D,     // 0xD0
    0x2D, 0xE5, 0x7A, 0x9F, 0x93, 0xC9, 0x9C, 0xEF,
    0xA0, 0xE0, 0x3B, 0x4D, 0xAE, 0x2A, 0xF5, 0xB0,     // 0xE0
    0xC8, 0xEB, 0xBB, 0x3C, 0x83, 0x53, 0x99, 0x61,
    0x17, 0x2B, 0x04, 0x7E, 0xBA, 0x77, 0xD6, 0x26,     // 0xF0
    0xE1, 0x69, 0x14, 0x63, 0x55, 0x21, 0x0C, 0x7D
};

// Rcon(i), 2^i in the Rijndael finite field, for i = 1..10.
// https://en.wikipedia.org/wiki/Rijndael_key_schedule
static unsigned char const rcon[10] = {
    0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1B, 0x36
};

static Sbox get_aes_sbox()
{
    return Sbox(sbox, sizeof(sbox));
}

static Sbox get_aes_inv_sbox()
{
    return Sbox(inv_sbox, sizeof(inv_sbox));
}

static void gen_aes128_setup_key(Code &code)
{
    // Shuffle pattern to rearrange the registers each round.
    static unsigned char const pattern[] = {
        4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3
    };

    // Set up the function prologue with 0 bytes of local variable storage.
    // X points to the key, and Z points to the key schedule.
    code.prologue_setup_key("aes_128_init", 0);

    // Write the number of rounds and the key size to the first 4 bytes.
    Reg sched = code.allocateReg(16);
    code.move(Reg(sched, 0, 4), 10 + ((176 + 4) << 16));
    code.stz(Reg(sched, 0, 4), POST_INC);

    // Load the key and write it to the first 16 bytes of the schedule.
    code.ldx(sched, POST_INC);
    code.stz(sched, 0);
    code.setFlag(Code::TempX);

    // We need the S-box pointer in Z, so move the schedule pointer to Y.
    code.move(Reg::y_ptr(), Reg::z_ptr());
    code.sbox_setup(0, get_aes_sbox());

    // Expand the key schedule until we have 176 bytes of expanded key.
    int iteration = 0;
    int n, w;
    Reg temp = code.allocateHighReg(1);
    for (n = 16, w = 4; n < 176; n += 4, ++w) {
        Reg s0  = Reg(sched, 0, 4);
        Reg s12 = Reg(sched, 12, 4);
        if (w == 4) {
            // Apply the key schedule core every 16 bytes / 4 words.
            code.sbox_lookup(temp, Reg(s12, 0, 1));
            code.logxor(Reg(s0, 3, 1), temp);

            code.sbox_lookup(temp, Reg(s12, 1, 1));
            code.logxor(Reg(s0, 0, 1), temp);
            code.move(temp, rcon[iteration++]);
            code.logxor(Reg(s0, 0, 1), temp);

            code.sbox_lookup(temp, Reg(s12, 2, 1));
            code.logxor(Reg(s0, 1, 1), temp);

            code.sbox_lookup(temp, Reg(s12, 3, 1));
            code.logxor(Reg(s0, 2, 1), temp);
            w = 0;
        } else {
            // XOR the word with the one 16 bytes previous.
            code.logxor(s0, s12);
        }
        code.sty(s0, 16);
        if ((n + 4) < 176)
            code.add_ptr_y(4);
        sched = sched.shuffle(pattern);
    }

    // Clean up and exit.
    code.sbox_cleanup();
}

static void gen_aes192_setup_key(Code &code)
{
    // Shuffle pattern to rearrange the registers each round.
    static unsigned char const pattern[] = {
        4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
        17, 18, 19, 20, 21, 22, 23, 0, 1, 2, 3
    };

    // Set up the function prologue with 0 bytes of local variable storage.
    // X points to the key, and Z points to the key schedule.
    code.prologue_setup_key("aes_192_init", 0);

    // Write the number of rounds and the key size to the first 4 bytes.
    Reg sched = code.allocateReg(24);
    code.move(Reg(sched, 0, 4), 12 + ((208 + 4) << 16));
    code.stz(Reg(sched, 0, 4), POST_INC);

    // Load the key and write it to the first 24 bytes of the schedule.
    code.ldx(sched, POST_INC);
    code.stz(sched, 0);
    code.setFlag(Code::TempX);

    // We need the S-box pointer in Z, so move the schedule pointer to Y.
    code.move(Reg::y_ptr(), Reg::z_ptr());
    code.sbox_setup(0, get_aes_sbox());

    // Expand the key schedule until we have 208 bytes of expanded key.
    int iteration = 0;
    int n, w;
    Reg temp = code.allocateHighReg(1);
    for (n = 24, w = 6; n < 208; n += 4, ++w) {
        Reg s0  = Reg(sched, 0, 4);
        Reg s20 = Reg(sched, 20, 4);
        if (w == 6) {
            // Apply the key schedule core every 24 bytes / 6 words.
            code.sbox_lookup(temp, Reg(s20, 0, 1));
            code.logxor(Reg(s0, 3, 1), temp);

            code.sbox_lookup(temp, Reg(s20, 1, 1));
            code.logxor(Reg(s0, 0, 1), temp);
            code.move(temp, rcon[iteration++]);
            code.logxor(Reg(s0, 0, 1), temp);

            code.sbox_lookup(temp, Reg(s20, 2, 1));
            code.logxor(Reg(s0, 1, 1), temp);

            code.sbox_lookup(temp, Reg(s20, 3, 1));
            code.logxor(Reg(s0, 2, 1), temp);
            w = 0;
        } else {
            // XOR the word with the one 24 bytes previous.
            code.logxor(s0, s20);
        }
        code.sty(s0, 24);
        if ((n + 4) < 208)
            code.add_ptr_y(4);
        sched = sched.shuffle(pattern);
    }

    // Clean up and exit.
    code.sbox_cleanup();
}

static void gen_aes256_setup_key(Code &code)
{
    // Set up the function prologue with 0 bytes of local variable storage.
    // X points to the key, and Z points to the key schedule.
    code.prologue_setup_key("aes_256_init", 0);

    // Write the number of rounds and the key size to the first 4 bytes.
    Reg s0 = code.allocateReg(4);
    code.move(s0, 14 + ((240 + 4) << 16));
    code.stz(s0, POST_INC);

    // Load the key and write it to the first 32 bytes of the schedule.
    Reg s28 = code.allocateReg(4);
    for (int offset = 0; offset < 32; offset += 4) {
        if (offset == 0) {
            code.ldx(s0, POST_INC);
            code.stz(s0, offset);
        } else {
            code.ldx(s28, POST_INC);
            code.stz(s28, offset);
        }
    }
    code.setFlag(Code::TempX);

    // We need the S-box pointer in Z, so move the schedule pointer to Y.
    code.move(Reg::y_ptr(), Reg::z_ptr());
    code.sbox_setup(0, get_aes_sbox());

    // Expand the key schedule until we have 240 bytes of expanded key.
    int iteration = 0;
    int n, w;
    Reg temp = code.allocateHighReg(1);
    for (n = 32, w = 8; n < 240; n += 4, ++w) {
        if (w == 8) {
            // Apply the key schedule core every 32 bytes / 8 words.
            code.sbox_lookup(temp, Reg(s28, 0, 1));
            code.logxor(Reg(s0, 3, 1), temp);

            code.sbox_lookup(temp, Reg(s28, 1, 1));
            code.logxor(Reg(s0, 0, 1), temp);
            code.move(temp, rcon[iteration++]);
            code.logxor(Reg(s0, 0, 1), temp);

            code.sbox_lookup(temp, Reg(s28, 2, 1));
            code.logxor(Reg(s0, 1, 1), temp);

            code.sbox_lookup(temp, Reg(s28, 3, 1));
            code.logxor(Reg(s0, 2, 1), temp);
            w = 0;
        } else if (w == 4) {
            // At the 16 byte mark we need to apply the S-box.
            code.sbox_lookup(temp, Reg(s28, 0, 1));
            code.logxor(Reg(s0, 0, 1), temp);

            code.sbox_lookup(temp, Reg(s28, 1, 1));
            code.logxor(Reg(s0, 1, 1), temp);

            code.sbox_lookup(temp, Reg(s28, 2, 1));
            code.logxor(Reg(s0, 2, 1), temp);

            code.sbox_lookup(temp, Reg(s28, 3, 1));
            code.logxor(Reg(s0, 3, 1), temp);
        } else {
            // XOR the word with the one 32 bytes previous.
            code.logxor(s0, s28);
        }

        // Store the word to the schedule and load the new schedule words.
        code.sty(s0, 32);
        if ((n + 4) < 240) {
            code.add_ptr_y(4);
            code.ldy(s0, 0);
            code.ldy(s28, 28);
        }
    }

    // Clean up and exit.
    code.sbox_cleanup();
}

// Applies the next round key to the state.
static void applyRoundKey(Code &code, const Reg &state, const Reg &temp)
{
    for (int offset = 0; offset < 16; ++offset) {
        code.ldx(temp, POST_INC);
        code.logxor(Reg(state, offset, 1), temp);
    }
}

// Applies the next round key to the state (decryption version).
static void inverseApplyRoundKey(Code &code, const Reg &state, const Reg &temp)
{
    for (int offset = 15; offset >= 0; --offset) {
        code.ldx(temp, PRE_DEC);
        code.logxor(Reg(state, offset, 1), temp);
    }
}

// Index a byte in the state by column and row.
#define S(col, row) (Reg(state, (col) * 4 + (row), 1))

// Apply the S-box and then shift the bytes of the rows.
static void subBytesAndShiftRows(Code &code, const Reg &state, const Reg &temp)
{
    // Map the bytes using the S-box and rearrange the state in-place.
    code.sbox_lookup(S(0, 0), S(0, 0));     // row0 <<<= 0
    code.sbox_lookup(S(1, 0), S(1, 0));
    code.sbox_lookup(S(2, 0), S(2, 0));
    code.sbox_lookup(S(3, 0), S(3, 0));

    code.sbox_lookup(temp, S(0, 1));        // row1 <<<= 8
    code.sbox_lookup(S(0, 1), S(1, 1));
    code.sbox_lookup(S(1, 1), S(2, 1));
    code.sbox_lookup(S(2, 1), S(3, 1));
    code.move(S(3, 1), temp);

    code.sbox_lookup(temp, S(0, 2));        // row2 <<<= 16
    code.sbox_lookup(S(0, 2), S(2, 2));
    code.move(S(2, 2), temp);
    code.sbox_lookup(temp, S(1, 2));
    code.sbox_lookup(S(1, 2), S(3, 2));
    code.move(S(3, 2), temp);

    code.sbox_lookup(temp, S(0, 3));        // row3 <<<= 24
    code.sbox_lookup(S(0, 3), S(3, 3));
    code.sbox_lookup(S(3, 3), S(2, 3));
    code.sbox_lookup(S(2, 3), S(1, 3));
    code.move(S(1, 3), temp);
}

// Apply the inverse S-box and then shift the bytes of the rows.
static void inverseSubBytesAndShiftRows
    (Code &code, const Reg &state, const Reg &temp)
{
    // Map the bytes using the inverse S-box and rearrange the state in-place.
    code.sbox_lookup(S(0, 0), S(0, 0));     // row0 >>>= 0
    code.sbox_lookup(S(1, 0), S(1, 0));
    code.sbox_lookup(S(2, 0), S(2, 0));
    code.sbox_lookup(S(3, 0), S(3, 0));

    code.sbox_lookup(temp, S(0, 1));        // row1 >>>= 8
    code.sbox_lookup(S(0, 1), S(3, 1));
    code.sbox_lookup(S(3, 1), S(2, 1));
    code.sbox_lookup(S(2, 1), S(1, 1));
    code.move(S(1, 1), temp);

    code.sbox_lookup(temp, S(0, 2));        // row2 >>>= 16
    code.sbox_lookup(S(0, 2), S(2, 2));
    code.move(S(2, 2), temp);
    code.sbox_lookup(temp, S(1, 2));
    code.sbox_lookup(S(1, 2), S(3, 2));
    code.move(S(3, 2), temp);

    code.sbox_lookup(temp, S(0, 3));        // row3 >>>= 24
    code.sbox_lookup(S(0, 3), S(1, 3));
    code.sbox_lookup(S(1, 3), S(2, 3));
    code.sbox_lookup(S(2, 3), S(3, 3));
    code.move(S(3, 3), temp);
}

// Double a byte value in the GF(2^8) field; "temp" must be a high register.
static void gdouble(Code &code, const Reg &a2, const Reg &a, const Reg &temp)
{
    code.move(a2, a);
    code.tworeg(Insn::MOV, temp.reg(0), ZERO_REG);
    code.lsl(a2, 1);
    code.tworeg(Insn::SBC, temp.reg(0), ZERO_REG);
    code.logand(temp, 0x1B);
    code.logxor(a2, temp);
}
static void gdouble(Code &code, const Reg &a, const Reg &temp)
{
    code.tworeg(Insn::MOV, temp.reg(0), ZERO_REG);
    code.lsl(a, 1);
    code.tworeg(Insn::SBC, temp.reg(0), ZERO_REG);
    code.logand(temp, 0x1B);
    code.logxor(a, temp);
}

// Apply MixColumns to a single column.
static void mixColumn(Code &code, const Reg &state, int col, const Reg &temp)
{
    Reg a = S(col, 0);
    Reg b = S(col, 1);
    Reg c = S(col, 2);
    Reg d = S(col, 3);
    Reg a2 = code.allocateReg(1);
    Reg b2 = code.allocateReg(1);
    Reg c2 = code.allocateReg(1);

    gdouble(code, a2, a, temp);
    gdouble(code, b2, b, temp);
    gdouble(code, c2, c, temp);

    // s0 = a2 ^ b2 ^ b ^ c ^ d;
    Reg s0_out = code.allocateReg(1);
    code.move(s0_out, a2);
    code.logxor(s0_out, b2);
    code.logxor(s0_out, b);
    code.logxor(s0_out, c);
    code.logxor(s0_out, d);

    // s1 = a ^ b2 ^ c2 ^ c ^ d;
    Reg s1_out = code.allocateReg(1);
    code.move(s1_out, a);
    code.logxor(s1_out, b2);
    code.logxor(s1_out, c2);
    code.logxor(s1_out, c);
    code.logxor(s1_out, d);

    // Can discard b2 now and reuse the register for d2.
    Reg d2 = b2;
    gdouble(code, d2, d, temp);

    // s2 = a ^ b ^ c2 ^ d2 ^ d;
    Reg s2_out = temp;
    code.move(s2_out, a);
    code.logxor(s2_out, b);
    code.logxor(s2_out, c2);
    code.logxor(s2_out, d2);
    code.logxor(s2_out, d);

    // s3 = a2 ^ a ^ b ^ c ^ d2;
    code.move(d, a2);
    code.logxor(d, a);
    code.logxor(d, b);
    code.logxor(d, c);
    code.logxor(d, d2);

    // Move the final s0, s1, and s2 values into place.
    code.move(a, s0_out);
    code.move(b, s1_out);
    code.move(c, s2_out);

    // Release all temporary registers.
    code.releaseReg(a2);
    code.releaseReg(b2);
    code.releaseReg(c2);
    code.releaseReg(s0_out);
    code.releaseReg(s1_out);
}

// Apply InverseMixColumns to a single column.
static void inverseMixColumn
    (Code &code, const Reg &state, int col, const Reg &temp)
{
    Reg a = S(col, 0);
    Reg b = S(col, 1);
    Reg c = S(col, 2);
    Reg d = S(col, 3);
    Reg a2 = code.allocateReg(1);
    Reg b2 = code.allocateReg(1);
    Reg c2 = code.allocateReg(1);

    gdouble(code, a2, a, temp);
    gdouble(code, b2, b, temp);
    gdouble(code, c2, c, temp);

    // s0 = a8 ^ a4 ^ a2 ^ b8 ^ b2 ^ b ^ c8 ^ c4 ^ c ^ d8 ^ d;
    //                xx        xx   x             x        x
    Reg s0_out = code.allocateReg(1);
    code.move(s0_out, a2);
    code.logxor(s0_out, b);
    code.logxor(s0_out, b2);
    code.logxor(s0_out, c);
    code.logxor(s0_out, d);

    // s1 = a8 ^ a ^ b8 ^ b4 ^ b2 ^ c8 ^ c2 ^ c ^ d8 ^ d4 ^ d;
    //           x             xx        xx   x             x
    Reg s1_out = code.allocateReg(1);
    code.move(s1_out, a);
    code.logxor(s1_out, c);
    code.logxor(s1_out, d);
    code.logxor(s1_out, b2);
    code.logxor(s1_out, c2);

    // s2 = a8 ^ a4 ^ a ^ b8 ^ b ^ c8 ^ c4 ^ c2 ^ d8 ^ d2 ^ d;
    //                x        x             xx        xx   x
    Reg s2_out = code.allocateReg(1);
    code.move(s2_out, a);
    code.logxor(s2_out, b);
    code.logxor(s2_out, d);
    code.logxor(s2_out, c2);
    gdouble(code, d, temp); // d = d2
    code.logxor(s2_out, d);

    // s3 = a8 ^ a2 ^ a ^ b8 ^ b4 ^ b ^ c8 ^ c ^ d8 ^ d4 ^ d2;
    //           xx   x             x        x             xx
    Reg s3_out = code.allocateReg(1);
    code.move(s3_out, a);
    code.logxor(s3_out, b);
    code.logxor(s3_out, c);
    code.logxor(s3_out, a2);
    code.logxor(s3_out, d);

    // compute a4, b4, c4, d4 (stored in a2, b2, c2, d)
    gdouble(code, a2, temp);
    gdouble(code, b2, temp);
    gdouble(code, c2, temp);
    gdouble(code, d, temp);

    // s0 = a8 ^ a4 ^ a2 ^ b8 ^ b2 ^ b ^ c8 ^ c4 ^ c ^ d8 ^ d;
    //           xx   xx        xx   x        xx   x        x
    code.logxor(s0_out, a2);
    code.logxor(s0_out, c2);

    // s1 = a8 ^ a ^ b8 ^ b4 ^ b2 ^ c8 ^ c2 ^ c ^ d8 ^ d4 ^ d;
    //           x        xx   xx        xx   x        xx   x
    code.logxor(s1_out, b2);
    code.logxor(s1_out, d);

    // s2 = a8 ^ a4 ^ a ^ b8 ^ b ^ c8 ^ c4 ^ c2 ^ d8 ^ d2 ^ d;
    //           xx   x        x        xx   xx        xx   x
    code.logxor(s2_out, a2);
    code.logxor(s2_out, c2);

    // s3 = a8 ^ a2 ^ a ^ b8 ^ b4 ^ b ^ c8 ^ c ^ d8 ^ d4 ^ d2;
    //           xx   x        xx   x        x        xx   xx
    code.logxor(s3_out, b2);
    code.logxor(s3_out, d);

    // compute a8, b8, c8, d8 (stored in a2, b2, c2, d)
    gdouble(code, a2, temp);
    gdouble(code, b2, temp);
    gdouble(code, c2, temp);
    gdouble(code, d, temp);

    // s0 = a8 ^ a4 ^ a2 ^ b8 ^ b2 ^ b ^ c8 ^ c4 ^ c ^ d8 ^ d;
    //      xx   xx   xx   xx   xx   x   xx   xx   x   xx   x
    code.logxor(s0_out, a2);
    code.logxor(s0_out, b2);
    code.logxor(s0_out, c2);
    code.logxor(s0_out, d);

    // s1 = a8 ^ a ^ b8 ^ b4 ^ b2 ^ c8 ^ c2 ^ c ^ d8 ^ d4 ^ d;
    //      xx   x   xx   xx   xx   xx   xx   x   xx   xx   x
    code.logxor(s1_out, a2);
    code.logxor(s1_out, b2);
    code.logxor(s1_out, c2);
    code.logxor(s1_out, d);

    // s2 = a8 ^ a4 ^ a ^ b8 ^ b ^ c8 ^ c4 ^ c2 ^ d8 ^ d2 ^ d;
    //      xx   xx   x   xx   x   xx   xx   xx   xx   xx   x
    code.logxor(s2_out, a2);
    code.logxor(s2_out, b2);
    code.logxor(s2_out, c2);
    code.logxor(s2_out, d);

    // s3 = a8 ^ a2 ^ a ^ b8 ^ b4 ^ b ^ c8 ^ c ^ d8 ^ d4 ^ d2;
    //      xx   xx   x   xx   xx   x   xx   x   xx   xx   xx
    code.logxor(s3_out, a2);
    code.logxor(s3_out, b2);
    code.logxor(s3_out, c2);
    code.logxor(s3_out, d);

    // Move the final s0, s1, s2, and s3 values into place.
    code.move(a, s0_out);
    code.move(b, s1_out);
    code.move(c, s2_out);
    code.move(d, s3_out);

    // Release all temporary registers.
    code.releaseReg(a2);
    code.releaseReg(b2);
    code.releaseReg(c2);
    code.releaseReg(s0_out);
    code.releaseReg(s1_out);
    code.releaseReg(s2_out);
    code.releaseReg(s3_out);
}

static void gen_aes_ecb_encrypt(Code &code)
{
    // Set up the function prologue with 0 bytes of local variable storage.
    // X will point to the input and Z points to the key schedule.
    code.prologue_encrypt_block("aes_ecb_encrypt", 0);

    // Allocate the registers that we need.
    Reg temp1 = code.allocateHighReg(1);
    Reg temp2 = code.allocateHighReg(1);
    Reg state = code.allocateReg(16);

    // Load the state into registers.
    code.ldx(state, POST_INC);

    // Transfer the key schedule to the X pointer and load the S-box pointer.
    code.ldz(temp1, 0);
    code.add_ptr_z(4);
    code.move(Reg::x_ptr(), Reg::z_ptr());
    code.sbox_setup(0, get_aes_sbox(), temp2);

    // XOR the state with the first round key.
    applyRoundKey(code, state, temp2);
    code.releaseReg(temp2);
    temp2 = Reg();

    // Determine the number of rounds to perform and skip ahead.
    unsigned char rounds_10 = 0;
    unsigned char rounds_12 = 0;
    code.compare(temp1, 10);
    code.breq(rounds_10);
    code.compare(temp1, 12);
    code.breq(rounds_12);

    // Unroll the outer part of the round loop.
    unsigned char subroutine = 0;
    unsigned char end_label = 0;
    for (int round = 0; round < 13; ++round) {
        if (round == 2)
            code.label(rounds_12);
        if (round == 4)
            code.label(rounds_10);
        code.call(subroutine);
    }
    subBytesAndShiftRows(code, state, temp1);
    applyRoundKey(code, state, temp1);
    code.jmp(end_label);

    // Subroutine for performing a main round.
    code.label(subroutine);
    subBytesAndShiftRows(code, state, temp1);
    mixColumn(code, state, 0, temp1);
    mixColumn(code, state, 1, temp1);
    mixColumn(code, state, 2, temp1);
    mixColumn(code, state, 3, temp1);
    applyRoundKey(code, state, temp1);
    code.ret();

    // Store the state to the output buffer.
    code.label(end_label);
    code.sbox_cleanup();
    code.load_output_ptr();
    code.stx(state, POST_INC);
}

static void gen_aes_ecb_decrypt(Code &code)
{
    // Set up the function prologue with 0 bytes of local variable storage.
    // X will point to the input and Z points to the key schedule.
    code.prologue_decrypt_block("aes_ecb_decrypt", 0);

    // Allocate the registers that we need.
    Reg temp1 = code.allocateHighReg(1);
    Reg temp2 = code.allocateHighReg(1);
    Reg state = code.allocateReg(16);

    // Load the state into registers.
    code.ldx(state, POST_INC);

    // Transfer the key schedule to the X pointer and load the S-box pointer.
    // X will point just past the end of the key schedule.
    code.ldz(temp1, 0);
    code.ldz(temp2, 2);
    code.add(Reg::z_ptr(), temp2);
    code.move(Reg::x_ptr(), Reg::z_ptr());
    code.sbox_setup(1, get_aes_inv_sbox(), temp2);

    // Reverse the final round.
    inverseApplyRoundKey(code, state, temp2);
    inverseSubBytesAndShiftRows(code, state, temp2);
    code.releaseReg(temp2);
    temp2 = Reg();

    // Determine the number of rounds to perform and skip ahead.
    unsigned char rounds_10 = 0;
    unsigned char rounds_12 = 0;
    code.compare(temp1, 10);
    code.breq(rounds_10);
    code.compare(temp1, 12);
    code.breq(rounds_12);

    // Unroll the outer part of the round loop.
    unsigned char subroutine = 0;
    unsigned char end_label = 0;
    for (int round = 0; round < 13; ++round) {
        if (round == 2)
            code.label(rounds_12);
        if (round == 4)
            code.label(rounds_10);
        code.call(subroutine);
    }
    code.jmp(end_label);

    // Subroutine for performing a decryption round.
    code.label(subroutine);
    inverseApplyRoundKey(code, state, temp1);
    inverseMixColumn(code, state, 0, temp1);
    inverseMixColumn(code, state, 1, temp1);
    inverseMixColumn(code, state, 2, temp1);
    inverseMixColumn(code, state, 3, temp1);
    inverseSubBytesAndShiftRows(code, state, temp1);
    code.ret();

    // Apply the final round key.
    code.label(end_label);
    inverseApplyRoundKey(code, state, temp1);

    // Store the state to the output buffer.
    code.sbox_cleanup();
    code.load_output_ptr();
    code.stx(state, POST_INC);
}

static bool test_aes128_setup_key(Code &code, const gencrypto::TestVector &vec)
{
    unsigned char schedule[244];
    unsigned char key[16];
    memset(schedule, 0, sizeof(schedule));
    if (!vec.populate(key, sizeof(key), "Key"))
        return false;
    code.exec_setup_key(schedule, sizeof(schedule), key, sizeof(key));
    return vec.check(schedule, sizeof(schedule), "Schedule_Bytes");
}

static bool test_aes192_setup_key(Code &code, const gencrypto::TestVector &vec)
{
    unsigned char schedule[244];
    unsigned char key[24];
    memset(schedule, 0, sizeof(schedule));
    if (!vec.populate(key, sizeof(key), "Key"))
        return false;
    code.exec_setup_key(schedule, sizeof(schedule), key, sizeof(key));
    return vec.check(schedule, sizeof(schedule), "Schedule_Bytes");
}

static bool test_aes256_setup_key(Code &code, const gencrypto::TestVector &vec)
{
    unsigned char schedule[244];
    unsigned char key[32];
    memset(schedule, 0, sizeof(schedule));
    if (!vec.populate(key, sizeof(key), "Key"))
        return false;
    code.exec_setup_key(schedule, sizeof(schedule), key, sizeof(key));
    return vec.check(schedule, sizeof(schedule), "Schedule_Bytes");
}

static bool test_aes_ecb_encrypt(Code &code, const gencrypto::TestVector &vec)
{
    unsigned char schedule[244];
    unsigned char plaintext[16];
    unsigned char ciphertext[16];
    memset(schedule, 0, sizeof(schedule));
    if (!vec.populate(schedule, sizeof(schedule), "Schedule_Bytes"))
        return false;
    if (!vec.populate(plaintext, sizeof(plaintext), "Plaintext"))
        return false;
    code.exec_encrypt_block(schedule, sizeof(schedule),
                            ciphertext, sizeof(ciphertext),
                            plaintext, sizeof(plaintext));
    return vec.check(ciphertext, sizeof(ciphertext), "Ciphertext");
}

static bool test_aes_ecb_decrypt(Code &code, const gencrypto::TestVector &vec)
{
    unsigned char schedule[244];
    unsigned char plaintext[16];
    unsigned char ciphertext[16];
    memset(schedule, 0, sizeof(schedule));
    if (!vec.populate(schedule, sizeof(schedule), "Schedule_Bytes"))
        return false;
    if (!vec.populate(ciphertext, sizeof(ciphertext), "Ciphertext"))
        return false;
    code.exec_encrypt_block(schedule, sizeof(schedule),
                            plaintext, sizeof(plaintext),
                            ciphertext, sizeof(ciphertext));
    return vec.check(plaintext, sizeof(plaintext), "Plaintext");
}

static void gen_aes_sboxes(Code &code)
{
    code.sbox_add(0, get_aes_sbox());
    code.sbox_add(1, get_aes_inv_sbox());
}

GENCRYPTO_REGISTER_AVR("aes_128_init", 0, "avr5",
                       gen_aes128_setup_key,
                       test_aes128_setup_key);
GENCRYPTO_REGISTER_AVR("aes_192_init", 0, "avr5",
                       gen_aes192_setup_key,
                       test_aes192_setup_key);
GENCRYPTO_REGISTER_AVR("aes_256_init", 0, "avr5",
                       gen_aes256_setup_key,
                       test_aes256_setup_key);
GENCRYPTO_REGISTER_AVR("aes_ecb_encrypt", 0, "avr5",
                       gen_aes_ecb_encrypt,
                       test_aes_ecb_encrypt);
GENCRYPTO_REGISTER_AVR("aes_ecb_decrypt", 0, "avr5",
                       gen_aes_ecb_decrypt,
                       test_aes_ecb_decrypt);
GENCRYPTO_REGISTER_AVR("aes_sboxes", 0, "avr5", gen_aes_sboxes, 0);
