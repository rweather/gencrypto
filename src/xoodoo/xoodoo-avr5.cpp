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

// Number of rounds for the Xoodoo permutation.
#define XOODOO_ROUNDS 12

// Round constants for Xoodoo.
static uint16_t const xoodoo_rc[XOODOO_ROUNDS] = {
    0x0058, 0x0038, 0x03C0, 0x00D0, 0x0120, 0x0014,
    0x0060, 0x002C, 0x0380, 0x00F0, 0x01A0, 0x0012
};

// Offset of a word in the Xoodoo state.
#define XOODOO_WORD(row, col) ((row) * 16 + (col) * 4)

static void gen_avr_xoodoo_permutation(Code &code)
{
    // Set up the function prologue with 0 bytes of local variable storage.
    // Z points to the permutation state on input and output.
    Reg count = code.prologue_permutation_with_count("xoodoo_permute", 0);
    code.setFlag(Code::TempY);

    // We need a 16-bit high register for the round constant.
    Reg rc = code.allocateHighReg(2);

    // Unroll the main loop with the bulk of the permutation in a subroutine.
    // Round count values of 12 and 6 are the most likely.
    unsigned char round_labels[XOODOO_ROUNDS] = {0};
    unsigned char subroutine = 0;
    unsigned char end_label = 0;
    code.compare(count, XOODOO_ROUNDS);
    code.breq(round_labels[0]);
    code.compare(count, 6);
    code.breq(round_labels[6]);
    for (int round = 1; round < XOODOO_ROUNDS; ++round) {
        if (round == 6)
            continue;
        code.compare(count, round);
        code.breq(round_labels[XOODOO_ROUNDS - round]);
    }
    code.jmp(end_label); // 0 rounds or > 12 rounds.
    code.releaseReg(count);
    for (int round = 1; round < XOODOO_ROUNDS; ++round) {
        if (round != 6)
            code.label(round_labels[round]);
        code.move(rc, xoodoo_rc[round]);
        code.call(subroutine);
    }
    code.jmp(end_label);

    // Special-case for 12 rounds which allows us to optimise the
    // loading of the round constants from one round to the next.
    code.label(round_labels[0]);
    for (int round = 0; round < XOODOO_ROUNDS; ++round) {
        if (round > 0 &&
              (xoodoo_rc[round] & 0xFF00) == (xoodoo_rc[round - 1] & 0xFF00)) {
            // The high byte is the same as last time so no need to change it.
            code.move(Reg(rc, 0, 1), xoodoo_rc[round]);
        } else {
            code.move(rc, xoodoo_rc[round]);
        }
        code.call(subroutine);
    }
    code.jmp(end_label);

    // Special-case for 6 rounds which allows us to optimise the
    // loading of the round constants from one round to the next.
    code.label(round_labels[6]);
    for (int round = 6; round < XOODOO_ROUNDS; ++round) {
        if (round > 6 &&
              (xoodoo_rc[round] & 0xFF00) == (xoodoo_rc[round - 1] & 0xFF00)) {
            // The high byte is the same as last time so no need to change it.
            code.move(Reg(rc, 0, 1), xoodoo_rc[round]);
        } else {
            code.move(rc, xoodoo_rc[round]);
        }
        code.call(subroutine);
    }
    code.jmp(end_label);

    // Start of the subroutine.
    code.label(subroutine);
    Reg x0 = code.allocateReg(4);
    Reg x1 = code.allocateReg(4);
    Reg x2 = code.allocateReg(4);
    Reg t1 = code.allocateReg(4);
    Reg t2 = code.allocateReg(4);
    Reg t3 = code.allocateReg(4);

    // Step theta: Mix column parity.
    // t1 = x03 ^ x13 ^ x23;
    code.ldz(t1, XOODOO_WORD(0, 3));
    code.ldz_xor(t1, XOODOO_WORD(1, 3));
    code.ldz_xor(t1, XOODOO_WORD(2, 3));
    // t2 = x00 ^ x10 ^ x20;
    code.ldz(x0, XOODOO_WORD(0, 0));
    code.ldz(x1, XOODOO_WORD(1, 0));
    code.ldz(x2, XOODOO_WORD(2, 0));
    code.move(t2, x0);
    code.logxor(t2, x1);
    code.logxor(t2, x2);
    // t1 = leftRotate5(t1) ^ leftRotate14(t1);
    // Do the calculation in a way that avoids physical byte rotations.
    Reg t1save = t1;
    code.move(t3, t1);
    code.ror(t1, 3);
    t1 = t1.shuffle(3, 0, 1, 2);
    code.ror(t3, 2);
    code.logxor(t1, t3.shuffle(2, 3, 0, 1));
    // t2 = leftRotate5(t2) ^ leftRotate14(t2);
    Reg t2save = t2;
    code.move(t3, t2);
    code.ror(t2, 3);
    t2 = t2.shuffle(3, 0, 1, 2);
    code.ror(t3, 2);
    code.logxor(t2, t3.shuffle(2, 3, 0, 1));
    // x00 ^= t1; x10 ^= t1; x20 ^= t1;
    code.logxor(x0, t1);
    code.logxor(x1, t1);
    code.logxor(x2, t1);
    code.stz(x0, XOODOO_WORD(0, 0));
    code.stz(x1, XOODOO_WORD(1, 0));
    code.stz(x2, XOODOO_WORD(2, 0));
    t1 = t1save;
    // t1 = x01 ^ x11 ^ x21;
    code.ldz(x0, XOODOO_WORD(0, 1));
    code.ldz(x1, XOODOO_WORD(1, 1));
    code.ldz(x2, XOODOO_WORD(2, 1));
    code.move(t1, x0);
    code.logxor(t1, x1);
    code.logxor(t1, x2);
    // t1 = leftRotate5(t1) ^ leftRotate14(t1);
    code.move(t3, t1);
    code.ror(t1, 3);
    t1 = t1.shuffle(3, 0, 1, 2);
    code.ror(t3, 2);
    code.logxor(t1, t3.shuffle(2, 3, 0, 1));
    // x01 ^= t2; x11 ^= t2; x21 ^= t2;
    code.logxor(x0, t2);
    code.logxor(x1, t2);
    code.logxor(x2, t2);
    code.stz(x0, XOODOO_WORD(0, 1));
    code.stz(x1, XOODOO_WORD(1, 1));
    code.stz(x2, XOODOO_WORD(2, 1));
    t2 = t2save;
    // t2 = x02 ^ x12 ^ x22;
    code.ldz(x0, XOODOO_WORD(0, 2));
    code.ldz(x1, XOODOO_WORD(1, 2));
    code.ldz(x2, XOODOO_WORD(2, 2));
    code.move(t2, x0);
    code.logxor(t2, x1);
    code.logxor(t2, x2);
    // t2 = leftRotate5(t2) ^ leftRotate14(t2);
    code.move(t3, t2);
    code.ror(t2, 3);
    t2 = t2.shuffle(3, 0, 1, 2);
    code.ror(t3, 2);
    code.logxor(t2, t3.shuffle(2, 3, 0, 1));
    // x02 ^= t1; x12 ^= t1; x22 ^= t1;
    code.logxor(x0, t1);
    code.logxor(x1, t1);
    code.logxor(x2, t1);
    code.stz(x0, XOODOO_WORD(0, 2));
    code.stz(x1, XOODOO_WORD(1, 2));
    code.stz(x2, XOODOO_WORD(2, 2));
    t1 = t1save;
    // x03 ^= t2; x13 ^= t2; x23 ^= t2;
    code.ldz_xor_in(t2, XOODOO_WORD(0, 3));
    code.ldz(t1, XOODOO_WORD(1, 3));
    code.logxor(t1, t2); // Leave x13 in t1 for use in rho-west below.
    code.ldz(t3, XOODOO_WORD(2, 3));
    code.logxor(t3, t2); // Leave x23 in t3 for use in rho-west below.
    t2 = t2save;

    // Step rho-west: Plane shift.
    // t1 = x13; x13 = x12; x12 = x11; x11 = x10; x10 = t1;
    code.ldz(t2, XOODOO_WORD(1, 2));
    code.stz(t2, XOODOO_WORD(1, 3));
    code.ldz(t2, XOODOO_WORD(1, 1));
    code.stz(t2, XOODOO_WORD(1, 2));
    code.ldz(t2, XOODOO_WORD(1, 0));
    code.stz(t2, XOODOO_WORD(1, 1));
    code.stz(t1, XOODOO_WORD(1, 0));
    // x20 = leftRotate11(x20);
    code.ldz(t1, XOODOO_WORD(2, 0));
    code.rol(t1, 11);
    code.stz(t1, XOODOO_WORD(2, 0));
    // x21 = leftRotate11(x21);
    code.ldz(t1, XOODOO_WORD(2, 1));
    code.rol(t1, 11);
    code.stz(t1, XOODOO_WORD(2, 1));
    // x22 = leftRotate11(x22);
    code.ldz(t1, XOODOO_WORD(2, 2));
    code.rol(t1, 11);
    code.stz(t1, XOODOO_WORD(2, 2));
    // x23 = leftRotate11(x23);
    code.rol(t3, 11);
    code.stz(t3, XOODOO_WORD(2, 3));

    // Step iota: Add the round constant to the state.
    code.ldz(x0, XOODOO_WORD(0, 0));
    code.logxor(x0, rc);

    // Step chi: Non-linear layer.
    for (int col = 0; col < 4; ++col) {
        // x0c ^= (~x1c) & x2c;
        if (col != 0)
            code.ldz(x0, XOODOO_WORD(0, col));
        code.ldz(x1, XOODOO_WORD(1, col));
        code.ldz(x2, XOODOO_WORD(2, col));
        code.move(t1, x2);
        code.logand_not(t1, x1);
        code.logxor(x0, t1);
        code.stz(x0, XOODOO_WORD(0, col));

        // x1c ^= (~x2c) & x0c;
        code.move(t1, x0);
        code.logand_not(t1, x2);
        code.logxor(x1, t1);
        code.stz(x1, XOODOO_WORD(1, col));

        // x2c ^= (~x0c) & x1c;
        code.logand_not(x1, x0);
        code.logxor(x2, x1);
        code.stz(x2, XOODOO_WORD(2, col));
    }

    // Step rho-east: Plane shift.
    // x10 = leftRotate1(x10);
    code.ldz(t1, XOODOO_WORD(1, 0));
    code.rol(t1, 1);
    code.stz(t1, XOODOO_WORD(1, 0));
    // x11 = leftRotate1(x11);
    code.ldz(t1, XOODOO_WORD(1, 1));
    code.rol(t1, 1);
    code.stz(t1, XOODOO_WORD(1, 1));
    // x12 = leftRotate1(x12);
    code.ldz(t1, XOODOO_WORD(1, 2));
    code.rol(t1, 1);
    code.stz(t1, XOODOO_WORD(1, 2));
    // x13 = leftRotate1(x13);
    code.ldz(t1, XOODOO_WORD(1, 3));
    code.rol(t1, 1);
    code.stz(t1, XOODOO_WORD(1, 3));
    // t1 = leftRotate8(x22);
    code.ldz(t1, XOODOO_WORD(2, 2));
    // t2 = leftRotate8(x23);
    code.ldz(t2, XOODOO_WORD(2, 3));
    // x22 = leftRotate8(x20);
    code.ldz(t3, XOODOO_WORD(2, 0));
    code.stz(t3.shuffle(3, 0, 1, 2), XOODOO_WORD(2, 2));
    // x23 = leftRotate8(x21);
    code.ldz(t3, XOODOO_WORD(2, 1));
    code.stz(t3.shuffle(3, 0, 1, 2), XOODOO_WORD(2, 3));
    // x20 = t1;
    code.stz(t1.shuffle(3, 0, 1, 2), XOODOO_WORD(2, 0));
    // x21 = t2;
    code.stz(t2.shuffle(3, 0, 1, 2), XOODOO_WORD(2, 1));

    // Return from the subroutine and end the function.
    code.ret();
    code.label(end_label);
}

static bool test_avr_xoodoo_permutation
    (Code &code, const gencrypto::TestVector &vec)
{
    int numRounds = vec.valueAsInt("Num_Rounds", 12);
    unsigned char state[48];
    if (numRounds < 0 || numRounds > 12)
        return false;
    if (!vec.populate(state, sizeof(state), "Input"))
        return false;
    code.exec_permutation(state, 48, numRounds);
    return vec.check(state, sizeof(state), "Output");
}

GENCRYPTO_REGISTER_AVR("xoodoo_permute", 0, "avr5",
                       gen_avr_xoodoo_permutation,
                       test_avr_xoodoo_permutation);
