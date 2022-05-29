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

static Reg shuffle_left(const Reg &in, int bytes)
{
    bytes %= 8;
    switch (bytes) {
    case 0: default: return in;
    case 1: return in.shuffle(7, 0, 1, 2, 3, 4, 5, 6);
    case 2: return in.shuffle(6, 7, 0, 1, 2, 3, 4, 5);
    case 3: return in.shuffle(5, 6, 7, 0, 1, 2, 3, 4);
    case 4: return in.shuffle(4, 5, 6, 7, 0, 1, 2, 3);
    case 5: return in.shuffle(3, 4, 5, 6, 7, 0, 1, 2);
    case 6: return in.shuffle(2, 3, 4, 5, 6, 7, 0, 1);
    case 7: return in.shuffle(1, 2, 3, 4, 5, 6, 7, 0);
    }
}

// Adjusts the Z pointer so that "posn" can be accessed by an offset
// from Z that is between 0 and 63.  We have to do this because we
// cannot easily access the entire 200 byte state from Z otherwise.
static void adjust_z_offset_to(Code &code, int &z_offset, int posn)
{
    if (posn != z_offset)
        code.add_ptr_z(posn - z_offset);
    z_offset = posn;
}
static void adjust_z_offset(Code &code, int &z_offset, int posn)
{
    int new_offset = z_offset;
    if (posn < z_offset)
        new_offset = posn & ~63;
    else if (posn >= (z_offset + 64))
        new_offset = posn & ~63;
    adjust_z_offset_to(code, z_offset, new_offset);
}

static void rho_pi_1600
    (Code &code, int out_posn, int rotate, int in_posn, int &z_offset)
{
    Reg temp = code.allocateReg(8);
    Reg out;
    adjust_z_offset(code, z_offset, in_posn);
    code.ldz(temp, in_posn - z_offset);
    int shift = (rotate % 8);
    if (shift == 0) {
        out = shuffle_left(temp, rotate / 8);
    } else if (shift <= 4) {
        code.rol(temp, shift);
        out = shuffle_left(temp, rotate / 8);
    } else {
        code.ror(temp, 8 - shift);
        out = shuffle_left(temp, ((rotate + 8) / 8));
    }
    adjust_z_offset(code, z_offset, out_posn);
    code.stz(out, out_posn - z_offset);
    code.releaseReg(temp);
}

/**
 * \brief Generates the AVR code for the Keccak-p[1600] permutation.
 *
 * \param code The code block to generate into.
 */
static void gen_avr_keccakp_1600_permutation(Code &code)
{
    static uint64_t const RC[24] = {
        0x0000000000000001ULL, 0x0000000000008082ULL, 0x800000000000808AULL,
        0x8000000080008000ULL, 0x000000000000808BULL, 0x0000000080000001ULL,
        0x8000000080008081ULL, 0x8000000000008009ULL, 0x000000000000008AULL,
        0x0000000000000088ULL, 0x0000000080008009ULL, 0x000000008000000AULL,
        0x000000008000808BULL, 0x800000000000008BULL, 0x8000000000008089ULL,
        0x8000000000008003ULL, 0x8000000000008002ULL, 0x8000000000000080ULL,
        0x000000000000800AULL, 0x800000008000000AULL, 0x8000000080008081ULL,
        0x8000000000008080ULL, 0x0000000080000001ULL, 0x8000000080008008ULL
    };
    int round, index, index2;
    int z_offset;

    // Set up the function prologue with 40 bytes of local variable storage.
    // Z points to the permutation state on input and output.
    code.prologue_permutation("keccakp_1600_permute", 40);

    // We cannot hold the entire 200-byte state in registers at once so we
    // deal with the data one row, column, or lane at a time.  Between rounds,
    // only A[0,0] is kept in registers.
    Reg A00 = code.allocateReg(8);

    // Unroll the outer loop to handle round constants with an inner
    // subroutine to handle the bulk of the permutation.
    #define posn_A(row, col) ((row) * 40 + (col) * 8)
    unsigned char subroutine = 0;
    unsigned char end_label = 0;
    z_offset = 0;
    code.ldz(A00, posn_A(0, 0)); // Pre-load A(0, 0) into registers.
    for (round = 0; round < 24; ++round) {
        // Perform the bulk of the round by calling the subroutine.
        code.call(subroutine);

        // Reload A(0, 0) and XOR the round constant into it.
        code.ldz(A00, posn_A(0, 0));
        code.logxor(A00, RC[round]);
    }
    unsigned char leapfrog = 0;
    code.jmp(leapfrog);

    // Step mapping theta.
    //      for i in 0..4:
    //          C[i] = A(0, i) ^ A(1, i) ^ A(2, i) ^ A(3, i) ^ A(4, i)
    //      for i in 0..4:
    //          D = C[(i + 4) % 5] ^ (C[(i + 1) % 5] <<< 1)
    //          for j in 0..4:
    //              A(j, i) ^= D
    code.label(subroutine);
    Reg C = code.allocateReg(8);
    for (index = 0; index < 5; ++index) {
        adjust_z_offset_to(code, z_offset, posn_A(0, index));
        if (index == 0)
            code.move(C, A00);
        else
            code.ldz(C, posn_A(0, index) - z_offset);
        code.ldz_xor(C, posn_A(1, index) - z_offset);
        adjust_z_offset_to(code, z_offset, posn_A(2, index));
        code.ldz_xor(C, posn_A(2, index) - z_offset);
        code.ldz_xor(C, posn_A(3, index) - z_offset);
        adjust_z_offset_to(code, z_offset, posn_A(4, index));
        code.ldz_xor(C, posn_A(4, index) - z_offset);
        code.stlocal(C, index * 8);
    }
    for (index = 0; index < 5; ++index) {
        code.ldlocal(C, ((index + 1) % 5) * 8);
        code.rol(C, 1);
        code.ldlocal_xor(C, ((index + 4) % 5) * 8);
        for (index2 = 0; index2 < 5; ++index2) {
            if (index == 0 && index2 == 0) {
                code.logxor(A00, C);
            } else {
                adjust_z_offset(code, z_offset, posn_A(index2, index));
                code.ldz_xor_in(C, posn_A(index2, index) - z_offset);
            }
        }
    }

    // Place a leapfrog here to help jmp(end_label) reach the end.
    unsigned char skip = 0;
    code.jmp(skip);
    code.label(leapfrog);
    unsigned char leapfrog2 = 0;
    code.jmp(leapfrog2);
    code.label(skip);

    // Step mappings rho and pi combined into a single step.
    adjust_z_offset(code, z_offset, posn_A(0, 0));
    code.stz(A00, posn_A(0, 0) - z_offset);
    code.ldz(C, posn_A(0, 1) - z_offset); // C = A(0, 1)
    rho_pi_1600(code, posn_A(0, 1), 44, posn_A(1, 1), z_offset);
    rho_pi_1600(code, posn_A(1, 1), 20, posn_A(1, 4), z_offset);
    rho_pi_1600(code, posn_A(1, 4), 61, posn_A(4, 2), z_offset);
    rho_pi_1600(code, posn_A(4, 2), 39, posn_A(2, 4), z_offset);
    rho_pi_1600(code, posn_A(2, 4), 18, posn_A(4, 0), z_offset);
    rho_pi_1600(code, posn_A(4, 0), 62, posn_A(0, 2), z_offset);
    rho_pi_1600(code, posn_A(0, 2), 43, posn_A(2, 2), z_offset);
    rho_pi_1600(code, posn_A(2, 2), 25, posn_A(2, 3), z_offset);
    rho_pi_1600(code, posn_A(2, 3),  8, posn_A(3, 4), z_offset);
    rho_pi_1600(code, posn_A(3, 4), 56, posn_A(4, 3), z_offset);
    rho_pi_1600(code, posn_A(4, 3), 41, posn_A(3, 0), z_offset);
    rho_pi_1600(code, posn_A(3, 0), 27, posn_A(0, 4), z_offset);
    rho_pi_1600(code, posn_A(0, 4), 14, posn_A(4, 4), z_offset);
    rho_pi_1600(code, posn_A(4, 4),  2, posn_A(4, 1), z_offset);
    rho_pi_1600(code, posn_A(4, 1), 55, posn_A(1, 3), z_offset);
    rho_pi_1600(code, posn_A(1, 3), 45, posn_A(3, 1), z_offset);
    rho_pi_1600(code, posn_A(3, 1), 36, posn_A(1, 0), z_offset);
    rho_pi_1600(code, posn_A(1, 0), 28, posn_A(0, 3), z_offset);
    rho_pi_1600(code, posn_A(0, 3), 21, posn_A(3, 3), z_offset);
    rho_pi_1600(code, posn_A(3, 3), 15, posn_A(3, 2), z_offset);
    rho_pi_1600(code, posn_A(3, 2), 10, posn_A(2, 1), z_offset);
    rho_pi_1600(code, posn_A(2, 1),  6, posn_A(1, 2), z_offset);
    rho_pi_1600(code, posn_A(1, 2),  3, posn_A(2, 0), z_offset);
    code.rol(C, 1);
    adjust_z_offset(code, z_offset, posn_A(2, 0));
    code.stz(C, posn_A(2, 0) - z_offset);
    code.releaseReg(C);

    // Place a leapfrog here to help jmp(end_label) reach the end.
    unsigned char skip2 = 0;
    code.jmp(skip2);
    code.label(leapfrog2);
    code.jmp(end_label);
    code.label(skip2);

    // Step mapping chi.
    //
    //   B = A
    //   for i in 0..4:
    //      for j in 0..4:
    //         A(j, i) = B(j, i) ^ ((~B(j, (i + 1) % 5)) & B(j, (i + 2) % 5))
    //
    // We compute this using an interleaving method.  We load five bytes
    // from the 5 words in a row of B and then compute the 5 output bytes
    // from that and store.  Then we move onto the next 5 bytes of each row.
    Reg B0 = code.allocateReg(1);
    Reg B1 = code.allocateReg(1);
    Reg B2 = code.allocateReg(1);
    Reg B3 = code.allocateReg(1);
    Reg B4 = code.allocateReg(1);
    Reg A = code.allocateReg(1);
    for (index = 0; index < 5; ++index) {
        adjust_z_offset_to(code, z_offset, posn_A(index, 0));
        for (index2 = 0; index2 < 8; ++index2) {
            // Load 5 bytes from the current row.
            code.ldz(B0, index2);
            code.ldz(B1, index2 + 8);
            code.ldz(B2, index2 + 16);
            code.ldz(B3, index2 + 24);
            code.ldz(B4, index2 + 32);

            // A0 = B0 ^ ((~B1) & B2)
            code.move(A, B1);
            code.lognot(A);
            code.logand(A, B2);
            code.logxor(A, B0);
            code.stz(A, index2);

            // A1 = B1 ^ ((~B2) & B3)
            code.move(A, B2);
            code.lognot(A);
            code.logand(A, B3);
            code.logxor(A, B1);
            code.stz(A, index2 + 8);

            // A2 = B2 ^ ((~B3) & B4)
            code.move(A, B3);
            code.lognot(A);
            code.logand(A, B4);
            code.logxor(A, B2);
            code.stz(A, index2 + 16);

            // A3 = B3 ^ ((~B4) & B0)
            code.move(A, B4);
            code.lognot(A);
            code.logand(A, B0);
            code.logxor(A, B3);
            code.stz(A, index2 + 24);

            // A4 = B4 ^ ((~B0) & B1)
            code.lognot(B0);
            code.logand(B0, B1);
            code.logxor(B0, B4);
            code.stz(B0, index2 + 32);
        }
    }
    code.releaseReg(B0);
    code.releaseReg(B1);
    code.releaseReg(B2);
    code.releaseReg(B3);
    code.releaseReg(B4);
    code.releaseReg(A);

    // End of the inner subroutine.  Move the Z pointer back to the
    // start of the state for the next round.
    adjust_z_offset_to(code, z_offset, 0);
    code.ret();

    // A(0, 0) is still in registers, so store it back.
    code.label(end_label);
    code.stz(A00, posn_A(0, 0));
}

static bool test_avr_keccakp_1600_permutation
    (Code &code, const gencrypto::TestVector &vec)
{
    unsigned char state[200];
    if (!vec.populate(state, sizeof(state), "Input"))
        return false;
    code.exec_permutation(state, 200);
    return vec.check(state, sizeof(state), "Output");
}

GENCRYPTO_REGISTER_AVR("keccakp_1600_permute", 0, "avr5",
                       gen_avr_keccakp_1600_permutation,
                       test_avr_keccakp_1600_permutation);
