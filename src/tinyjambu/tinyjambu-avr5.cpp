/*
 * Copyright (C) 2021 Southern Storm Software, Pty Ltd.
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

static void gen_tinyjambu_steps_32
    (Code &code, const Reg &s0, const Reg &s1, const Reg &s2, const Reg &s3,
     int koffset)
{
    // Allocate some temporary working registers.  After the allocations
    // in the gen_tinyjambu_permutation() function we have 9 left spare.
    Reg temp = code.allocateReg(9);

    // t1 = (s1 >> 15) | (s2 << 17);
    // s0 ^= t1;
    code.move(Reg(temp, 2, 2), Reg(s1, 2, 2));
    code.move(Reg(temp, 4, 2), Reg(s2, 0, 2));
    code.move(Reg(temp, 1, 1), Reg(s1, 1, 1));
    code.lsl(Reg(temp, 1, 5), 1);
    code.logxor(s0, Reg(temp, 2, 4));

    // t2 = (s2 >> 6)  | (s3 << 26);
    // t3 = (s2 >> 21) | (s3 << 11);
    // s0 ^= ~(t2 & t3);
    // Note: We assume that the key is inverted so we can avoid the NOT.
    code.move(Reg(temp, 4, 4), s2);
    code.move(Reg(temp, 8, 1), s3);
    code.lsl(Reg(temp, 4, 5), 2);
    Reg t2 = Reg(temp, 5, 4);
    code.move(Reg(temp, 0, 2), Reg(s2, 2, 2));
    code.move(Reg(temp, 2, 3), Reg(s3, 0, 3));
    code.lsl(Reg(temp, 0, 5), 3);
    Reg t3 = Reg(temp, 1, 4);
    code.logand(t2, t3);
    code.logxor(s0, t2);

    // t4 = (s2 >> 27) | (s3 << 5);
    // s0 ^= t4;
    code.move(Reg(temp, 2, 4), s3);
    code.move(Reg(temp, 1, 1), Reg(s2, 3, 1));
    code.lsr(Reg(temp, 1, 5), 3);
    code.logxor(s0, Reg(temp, 1, 4));

    // s0 ^= k[koffset];
    code.ldz_xor(s0, 16 + koffset * 4);

    // Release the temporary working registers.
    code.releaseReg(temp);
}

/**
 * \brief Generates the AVR code for the TinyJAMBU permutation.
 *
 * \param code The code block to generate into.
 * \param name Name of the function to generate.
 * \param key_words Number of words in the key: 4, 6, or 8.
 */
static void gen_tinyjambu_permutation
    (Code &code, const char *name, int key_words)
{
    // Set up the function prologue.  Z points to the state.
    Reg rounds = code.prologue_permutation_with_count(name, 0);
    code.setFlag(Code::NoLocals);

    // Load the 128-bit state from Z into registers.
    Reg s0 = code.allocateReg(4);
    Reg s1 = code.allocateReg(4);
    Reg s2 = code.allocateReg(4);
    Reg s3 = code.allocateReg(4);
    code.ldz(s0, 0);
    code.ldz(s1, 4);
    code.ldz(s2, 8);
    code.ldz(s3, 12);

    // Perform all permutation rounds.  Each round has 128 steps
    // but it may be unrolled 2 or 3 times based on the key size.
    unsigned char top_label = 0;
    unsigned char end_label = 0;
    code.label(top_label);

    // Unroll the inner part of the loop.
    int inner_rounds;
    if (key_words == 4)
        inner_rounds = 1;
    else if (key_words == 6)
        inner_rounds = 3;
    else
        inner_rounds = 2;
    for (int inner = 0; inner < inner_rounds; ++inner) {
        // Perform the 128 steps of this inner round, 32 at a time.
        int koffset = inner * 4;
        gen_tinyjambu_steps_32(code, s0, s1, s2, s3, koffset % key_words);
        gen_tinyjambu_steps_32(code, s1, s2, s3, s0, (koffset + 1) % key_words);
        gen_tinyjambu_steps_32(code, s2, s3, s0, s1, (koffset + 2) % key_words);
        gen_tinyjambu_steps_32(code, s3, s0, s1, s2, (koffset + 3) % key_words);

        // Check for early bail-out between the inner rounds.
        if (inner < (inner_rounds - 1)) {
            code.dec(rounds);
            code.breq(end_label);
        }
    }

    // Decrement the round counter at the bottom of the round loop.
    code.dec(rounds);
    code.brne(top_label);

    // Store the 128-bit state in the registers back to Z.
    code.label(end_label);
    code.stz(s0, 0);
    code.stz(s1, 4);
    code.stz(s2, 8);
    code.stz(s3, 12);
}

/**
 * \brief Generates the AVR code for the TinyJAMBU-128 permutation.
 *
 * \param code The code block to generate into.
 */
static void gen_avr_tinyjambu_permutation_128(Code &code)
{
    gen_tinyjambu_permutation(code, "tinyjambu_permutation_128", 4);
}

/**
 * \brief Generates the AVR code for the TinyJAMBU-192 permutation.
 *
 * \param code The code block to generate into.
 */
static void gen_avr_tinyjambu_permutation_192(Code &code)
{
    gen_tinyjambu_permutation(code, "tinyjambu_permutation_192", 6);
}

/**
 * \brief Generates the AVR code for the TinyJAMBU-256 permutation.
 *
 * \param code The code block to generate into.
 */
static void gen_avr_tinyjambu_permutation_256(Code &code)
{
    gen_tinyjambu_permutation(code, "tinyjambu_permutation_256", 8);
}

/**
 * \brief Inverts a TinyJAMBU key.
 *
 * \param out Output key.
 * \param in Input key.
 * \param count Number of bytes in the key.
 */
static void invert_key
    (unsigned char *out, const unsigned char *in, unsigned count)
{
    while (count > 0) {
        *out++ = ~(*in++);
        --count;
    }
}

static bool test_avr_tinyjambu_permutation_128
    (Code &code, const gencrypto::TestVector &vec)
{
    unsigned char state[32];
    unsigned char key[16];
    if (!vec.populate(state, 16, "Input"))
        return false;
    if (!vec.populate(key, sizeof(key), "Key"))
        return false;
    invert_key(state + 16, key, 16);
    code.exec_permutation(state, 32, 1024 / 128);
    return vec.check(state, 16, "Output");
}

static bool test_avr_tinyjambu_permutation_192
    (Code &code, const gencrypto::TestVector &vec)
{
    unsigned char state[40];
    unsigned char key[24];
    if (!vec.populate(state, 16, "Input"))
        return false;
    if (!vec.populate(key, sizeof(key), "Key"))
        return false;
    invert_key(state + 16, key, 24);
    code.exec_permutation(state, 40, 1152 / 128);
    return vec.check(state, 16, "Output");
}

static bool test_avr_tinyjambu_permutation_256
    (Code &code, const gencrypto::TestVector &vec)
{
    unsigned char state[48];
    unsigned char key[32];
    if (!vec.populate(state, 16, "Input"))
        return false;
    if (!vec.populate(key, sizeof(key), "Key"))
        return false;
    invert_key(state + 16, key, 32);
    code.exec_permutation(state, 48, 1280 / 128);
    return vec.check(state, 16, "Output");
}

GENCRYPTO_REGISTER_AVR("tinyjambu_permutation_128", 0, "avr5",
                       gen_avr_tinyjambu_permutation_128,
                       test_avr_tinyjambu_permutation_128);
GENCRYPTO_REGISTER_AVR("tinyjambu_permutation_192", 0, "avr5",
                       gen_avr_tinyjambu_permutation_192,
                       test_avr_tinyjambu_permutation_192);
GENCRYPTO_REGISTER_AVR("tinyjambu_permutation_256", 0, "avr5",
                       gen_avr_tinyjambu_permutation_256,
                       test_avr_tinyjambu_permutation_256);
