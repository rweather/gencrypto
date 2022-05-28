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

static void rho_pi_200
    (Code &code, const Reg &out_reg, int rotate, const Reg &in_reg)
{
    code.rol(in_reg, rotate);
    code.move(out_reg, in_reg);
}

/**
 * \brief Generates the AVR code for the Keccak-p[200] permutation.
 *
 * \param code The code block to generate into.
 */
static void gen_avr_keccakp_200_permutation(Code &code)
{
    static uint8_t const RC[18] = {
        0x01, 0x82, 0x8A, 0x00, 0x8B, 0x01, 0x81, 0x09,
        0x8A, 0x88, 0x09, 0x0A, 0x8B, 0x8B, 0x89, 0x03,
        0x02, 0x80
    };
    int round, index, index2;

    // Set up the function prologue with 0 bytes of local variable storage.
    // Z points to the permutation state on input and output.
    code.prologue_permutation("keccakp_200_permute", 0);
    code.setFlag(Code::TempY);

    // Allocate 25 bytes for the core state and load it from Z.
    Reg A = code.allocateReg(25);
    code.ldz(A, 0);

    // Push Z on the stack so we can use it for temporaries.
    code.push(Reg::z_ptr());
    code.setFlag(Code::TempZ);

    // Allocate 5 bytes for the "C" array.  We force C[0] to be a high
    // register because we need one for loading round constants below.
    Reg C[5];
    C[0] = code.allocateHighReg(1);
    C[1] = code.allocateReg(1);
    C[2] = code.allocateReg(1);
    C[3] = code.allocateReg(1);
    C[4] = code.allocateReg(1);

    // Unroll the outer loop to handle round constants with an inner
    // subroutine to handle the bulk of the permutation.
    #define state_A(row, col) (Reg(A, (row) * 5 + (col), 1))
    unsigned char subroutine = 0;
    unsigned char end_label = 0;
    for (round = 0; round < 18; ++round) {
        code.call(subroutine);
        code.move(C[0], RC[round]);
        code.logxor(state_A(0, 0), C[0]);
    }
    code.jmp(end_label);

    // Step mapping theta.
    code.label(subroutine);
    for (index = 0; index < 5; ++index) {
        code.move(C[index], state_A(0, index));
        code.logxor(C[index], state_A(1, index));
        code.logxor(C[index], state_A(2, index));
        code.logxor(C[index], state_A(3, index));
        code.logxor(C[index], state_A(4, index));
    }
    for (index = 0; index < 5; ++index) {
        code.tworeg(Insn::MOV, TEMP_REG, C[(index + 1) % 5].reg(0));
        code.onereg(Insn::LSL, TEMP_REG); // Left rotate by 1 bit.
        code.tworeg(Insn::ADC, TEMP_REG, ZERO_REG);
        code.tworeg(Insn::EOR, TEMP_REG, C[(index + 4) % 5].reg(0));
        for (index2 = 0; index2 < 5; ++index2)
            code.tworeg(Insn::EOR, state_A(index2, index).reg(0), TEMP_REG);
    }

    // Step mappings rho and pi combined into a single step.
    code.move(C[0], state_A(0, 1));
    rho_pi_200(code, state_A(0, 1), 4, state_A(1, 1));
    rho_pi_200(code, state_A(1, 1), 4, state_A(1, 4));
    rho_pi_200(code, state_A(1, 4), 5, state_A(4, 2));
    rho_pi_200(code, state_A(4, 2), 7, state_A(2, 4));
    rho_pi_200(code, state_A(2, 4), 2, state_A(4, 0));
    rho_pi_200(code, state_A(4, 0), 6, state_A(0, 2));
    rho_pi_200(code, state_A(0, 2), 3, state_A(2, 2));
    rho_pi_200(code, state_A(2, 2), 1, state_A(2, 3));
    rho_pi_200(code, state_A(2, 3), 0, state_A(3, 4));
    rho_pi_200(code, state_A(3, 4), 0, state_A(4, 3));
    rho_pi_200(code, state_A(4, 3), 1, state_A(3, 0));
    rho_pi_200(code, state_A(3, 0), 3, state_A(0, 4));
    rho_pi_200(code, state_A(0, 4), 6, state_A(4, 4));
    rho_pi_200(code, state_A(4, 4), 2, state_A(4, 1));
    rho_pi_200(code, state_A(4, 1), 7, state_A(1, 3));
    rho_pi_200(code, state_A(1, 3), 5, state_A(3, 1));
    rho_pi_200(code, state_A(3, 1), 4, state_A(1, 0));
    rho_pi_200(code, state_A(1, 0), 4, state_A(0, 3));
    rho_pi_200(code, state_A(0, 3), 5, state_A(3, 3));
    rho_pi_200(code, state_A(3, 3), 7, state_A(3, 2));
    rho_pi_200(code, state_A(3, 2), 2, state_A(2, 1));
    rho_pi_200(code, state_A(2, 1), 6, state_A(1, 2));
    rho_pi_200(code, state_A(1, 2), 3, state_A(2, 0));
    code.rol(C[0], 1);
    code.move(state_A(2, 0), C[0]);

    // Step mapping chi.
    for (index = 0; index < 5; ++index) {
        code.move(C[0], state_A(index, 0));
        code.move(C[1], state_A(index, 1));
        code.move(C[2], state_A(index, 2));
        code.move(C[3], state_A(index, 3));
        code.move(C[4], state_A(index, 4));
        for (index2 = 0; index2 < 5; ++index2) {
            Reg s = state_A(index, index2);
            code.move(s, C[(index2 + 2) % 5]);
            code.logand_not(s, C[(index2 + 1) % 5]);
            code.logxor(s, C[index2]);
        }
    }

    // End of the inner subroutine.
    code.ret();

    // Restore Z from the stack and store the "A" state back again.
    code.label(end_label);
    code.pop(Reg::z_ptr());
    code.stz(A, 0);
}

static bool test_avr_keccakp_200_permutation
    (Code &code, const gencrypto::TestVector &vec)
{
    unsigned char state[25];
    if (!vec.populate(state, sizeof(state), "Input"))
        return false;
    code.exec_permutation(state, 25);
    return vec.check(state, sizeof(state), "Output");
}

GENCRYPTO_REGISTER_AVR("keccakp_200_permute", 0, "avr5",
                       gen_avr_keccakp_200_permutation,
                       test_avr_keccakp_200_permutation);
