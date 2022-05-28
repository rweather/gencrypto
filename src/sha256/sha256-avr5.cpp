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

// Round constants for SHA256.
static unsigned long const k[64] = {
    0x428a2f98U, 0x71374491U, 0xb5c0fbcfU, 0xe9b5dba5U,
    0x3956c25bU, 0x59f111f1U, 0x923f82a4U, 0xab1c5ed5U,
    0xd807aa98U, 0x12835b01U, 0x243185beU, 0x550c7dc3U,
    0x72be5d74U, 0x80deb1feU, 0x9bdc06a7U, 0xc19bf174U,
    0xe49b69c1U, 0xefbe4786U, 0x0fc19dc6U, 0x240ca1ccU,
    0x2de92c6fU, 0x4a7484aaU, 0x5cb0a9dcU, 0x76f988daU,
    0x983e5152U, 0xa831c66dU, 0xb00327c8U, 0xbf597fc7U,
    0xc6e00bf3U, 0xd5a79147U, 0x06ca6351U, 0x14292967U,
    0x27b70a85U, 0x2e1b2138U, 0x4d2c6dfcU, 0x53380d13U,
    0x650a7354U, 0x766a0abbU, 0x81c2c92eU, 0x92722c85U,
    0xa2bfe8a1U, 0xa81a664bU, 0xc24b8b70U, 0xc76c51a3U,
    0xd192e819U, 0xd6990624U, 0xf40e3585U, 0x106aa070U,
    0x19a4c116U, 0x1e376c08U, 0x2748774cU, 0x34b0bcb5U,
    0x391c0cb3U, 0x4ed8aa4aU, 0x5b9cca4fU, 0x682e6ff3U,
    0x748f82eeU, 0x78a5636fU, 0x84c87814U, 0x8cc70208U,
    0x90befffaU, 0xa4506cebU, 0xbef9a3f7U, 0xc67178f2U
};

struct sha256_state
{
    // Size of the local stack frame (24 or 32).
    int local_size;

    // Offsets of words in local storage.
    int a, b, c, d, e, f, g, h;

    // Registers holding the state and temporary variables.
    Reg areg, ereg;
    Reg temp1, temp2, temp3, temp4;
};

// Loads the state into registers and stack, and prepare for the rounds.
static void gen_sha256_load
    (Code &code, struct sha256_state &st, int local_size)
{
    // Allocate the registers we will need later.
    st.temp3 = code.allocateHighReg(4);
    st.temp1 = code.allocateReg(4);
    st.temp2 = code.allocateReg(4);
    st.temp4 = code.allocateReg(4);
    st.areg = code.allocateReg(4);
    st.ereg = code.allocateReg(4);

    // Offsets of the hash state words in local storage.
    // The "a" and "e" words are kept in registers, so not stored.
    st.local_size = local_size;
    if (local_size == 24) {
        st.a = -1;
        st.b = 0;
        st.c = 4;
        st.d = 8;
        st.e = -1;
        st.f = 12;
        st.g = 16;
        st.h = 20;
    } else {
        st.a = 0;
        st.b = 4;
        st.c = 8;
        st.d = 12;
        st.e = 16;
        st.f = 20;
        st.g = 24;
        st.h = 28;
    }

    // Load the hash state into local variables as we need to
    // preserve the original state until the end of the function.
    code.ldz(st.areg, 0);           // a
    code.ldz(st.temp1, 4);          // b
    code.stlocal(st.temp1, st.b);
    code.ldz(st.temp1, 8);          // c
    code.stlocal(st.temp1, st.c);
    code.ldz(st.temp1, 12);         // d
    code.stlocal(st.temp1, st.d);
    code.ldz(st.ereg, 16);          // e
    code.ldz(st.temp1, 20);         // f
    code.stlocal(st.temp1, st.f);
    code.ldz(st.temp1, 24);         // g
    code.stlocal(st.temp1, st.g);
    code.ldz(st.temp1, 28);         // h
    code.stlocal(st.temp1, st.h);

    // Advance Z to point to the "w" state array so that we can index it
    // with offsets between 0 and 63.
    code.add_ptr_z(32);
}

// Store the computed hash back to the state at the end of the process.
static void gen_sha256_store(Code &code, struct sha256_state &st)
{
    // Add the local hash state to the original hash state.
    // Note that "a" and "e" are still in registers.
    code.sub_ptr_z(32);
    code.ldz(st.temp1, 0);      // a
    code.add(st.areg, st.temp1);
    code.stz(st.areg, 0);
    code.ldz(st.temp1, 4);      // b
    code.ldlocal(st.temp2, st.b);
    code.add(st.temp1, st.temp2);
    code.stz(st.temp1, 4);
    code.ldz(st.temp1, 8);      // c
    code.ldlocal(st.temp2, st.c);
    code.add(st.temp1, st.temp2);
    code.stz(st.temp1, 8);
    code.ldz(st.temp1, 12);     // d
    code.ldlocal(st.temp2, st.d);
    code.add(st.temp1, st.temp2);
    code.stz(st.temp1, 12);
    code.ldz(st.temp1, 16);     // e
    code.add(st.ereg, st.temp1);
    code.stz(st.ereg, 16);
    code.ldz(st.temp1, 20);     // f
    code.ldlocal(st.temp2, st.f);
    code.add(st.temp1, st.temp2);
    code.stz(st.temp1, 20);
    code.ldz(st.temp1, 24);     // g
    code.ldlocal(st.temp2, st.g);
    code.add(st.temp1, st.temp2);
    code.stz(st.temp1, 24);
    code.ldz(st.temp1, 28);     // h
    code.ldlocal(st.temp2, st.h);
    code.add(st.temp1, st.temp2);
    code.stz(st.temp1, 28);
}

// Generate a single step for computing temp1 and temp2.
//
// It is assumed that "temp1" already contains the state word,
// and that "temp3" already contains the round constant.
static void gen_sha256_step(Code &code, struct sha256_state &st)
{
    // temp1 = h + k[index] + state->w[index] +
    //    (rightRotate6(e) ^ rightRotate11(e) ^ rightRotate25(e)) +
    //    ((e & f) ^ ((~e) & g));
    code.add(st.temp1, st.temp3);
    code.ldlocal(st.temp2, st.h);
    code.add(st.temp1, st.temp2);
    // temp1 += rightRotate6(e) ^ rightRotate11(e) ^ rightRotate25(e);
    code.move(st.temp2, st.ereg);
    code.rol(st.temp2, 2); // 6 = 8 - 2
    code.move(st.temp3, st.ereg);
    code.ror(st.temp3, 3); // 11 = 8 + 3
    code.logxor(st.temp2.shuffle(1, 2, 3, 0), st.temp3.shuffle(1, 2, 3, 0));
    code.move(st.temp3, st.ereg);
    code.ror(st.temp3, 1); // 25 = 24 + 1
    code.logxor(st.temp2.shuffle(1, 2, 3, 0), st.temp3.shuffle(3, 0, 1, 2));
    code.add(st.temp1, st.temp2.shuffle(1, 2, 3, 0));
    // temp1 += ((e & f) ^ ((~e) & g));
    code.ldlocal(st.temp2, st.f);
    code.logand(st.temp2, st.ereg);
    code.ldlocal(st.temp3, st.g);
    code.move(st.temp4, st.ereg);
    code.lognot(st.temp4);
    code.logand(st.temp3, st.temp4);
    code.logxor(st.temp2, st.temp3);
    code.add(st.temp1, st.temp2);

    // temp2 = (rightRotate2(a) ^ rightRotate13(a) ^ rightRotate22(a)) +
    //    ((a & b) ^ (a & c) ^ (b & c));
    code.move(st.temp2, st.areg);
    code.ror(st.temp2, 2);
    code.move(st.temp3, st.areg);
    code.rol(st.temp3, 3); // 13 = 16 - 3
    code.logxor(st.temp2, st.temp3.shuffle(2, 3, 0, 1));
    code.move(st.temp3, st.areg);
    code.rol(st.temp3, 2); // 22 = 24 - 2
    code.logxor(st.temp2, st.temp3.shuffle(3, 0, 1, 2));
    code.ldlocal(st.temp3, st.b);
    code.ldlocal(st.temp4, st.c);
    code.logand(st.temp4, st.temp3);
    code.logand(st.temp3, st.areg);
    code.logxor(st.temp3, st.temp4);
    code.ldlocal(st.temp4, st.c);
    code.logand(st.temp4, st.areg);
    code.logxor(st.temp3, st.temp4);
    code.add(st.temp2, st.temp3);
}

// Rotates the hash state virtually by rearranging the registers.
static void gen_sha256_rotate(Code &code, struct sha256_state &st)
{
    if (st.local_size == 24) {
        int hh = st.h;
        st.h = st.g;
        st.g = st.f;
        // f = e;
        st.f = hh;
        code.stlocal(st.ereg, st.f);
        // e = d + temp1;
        code.ldlocal(st.ereg, st.d);
        code.add(st.ereg, st.temp1);
        int dd = st.d;
        st.d = st.c;
        st.c = st.b;
        // b = a;
        st.b = dd;
        code.stlocal(st.areg, st.b);
        // a = temp1 + temp2;
        code.move(st.areg, st.temp1);
        code.add(st.areg, st.temp2);
    } else {
        // store a and e back to local variables
        code.stlocal(st.areg, st.a);
        code.stlocal(st.ereg, st.e);

        // e = d + temp1
        code.ldlocal(st.ereg, st.d);
        code.add(st.ereg, st.temp1);

        // a = temp1 + temp2
        code.move(st.areg, st.temp1);
        code.add(st.areg, st.temp2);

        // rotate the offsets
        int hh = st.h;
        st.h = st.g;
        st.g = st.f;
        st.f = st.e;
        st.e = st.d;
        st.d = st.c;
        st.c = st.b;
        st.b = st.a;
        st.a = hh;
    }
}

// Perform a full rotation of the hash state.
static void gen_sha256_rotate_full(Code &code, struct sha256_state &st)
{
    // The "a" and "e" values are in registers on entry and exit.
    // Everything else is in the local stack frame.

    // h = g;
    code.ldlocal(st.temp3, st.g);
    code.stlocal(st.temp3, st.h);

    // g = f;
    code.ldlocal(st.temp3, st.f);
    code.stlocal(st.temp3, st.g);

    // f = e;
    code.stlocal(st.ereg, st.f);

    // e = d + temp1;
    code.ldlocal(st.ereg, st.d);
    code.add(st.ereg, st.temp1);

    // d = c;
    code.ldlocal(st.temp3, st.c);
    code.stlocal(st.temp3, st.d);

    // c = b;
    code.ldlocal(st.temp3, st.b);
    code.stlocal(st.temp3, st.c);

    // b = a;
    code.stlocal(st.areg, st.b);

    // a = temp1 + temp2;
    code.move(st.areg, st.temp1);
    code.add(st.areg, st.temp2);
}

// Derives a state word for rounds 17..64.
static void gen_sha256_derive_state_word
    (Code &code, struct sha256_state &st, int index)
{
    // temp1 = state->w[(index - 15) & 0x0F];
    code.ldz(st.temp1.reversed(), ((index - 15) * 4) & 0x3F);

    // temp2 = state->w[(index - 2) & 0x0F];
    code.ldz(st.temp2.reversed(), ((index - 2) * 4) & 0x3F);

    // temp1 = state->w[index & 0x0F] =
    //   state->w[(index - 16) & 0x0F] + state->w[(index - 7) & 0x0F] +
    //   (rightRotate7(temp1) ^ rightRotate18(temp1) ^ (temp1 >> 3)) +
    //   (rightRotate17(temp2) ^ rightRotate19(temp2) ^ (temp2 >> 10));
    code.move(st.temp3, st.temp1);
    code.rol(st.temp3, 1); // 7 = 8 - 1
    code.move(st.temp4, st.temp1);
    code.ror(st.temp4, 2); // 18 = 16 + 2
    code.lsr(st.temp1, 3);
    code.logxor(st.temp1, st.temp3.shuffle(1, 2, 3, 0));
    code.logxor(st.temp1, st.temp4.shuffle(2, 3, 0, 1));
    code.move(st.temp3, st.temp2);
    code.ror(st.temp3, 1); // 17 = 16 + 1
    code.move(st.temp4, st.temp2);
    code.ror(st.temp4, 3); // 19 = 16 + 3
    code.lsr(st.temp2, 10);
    code.logxor(st.temp2, st.temp3.shuffle(2, 3, 0, 1));
    code.logxor(st.temp2, st.temp4.shuffle(2, 3, 0, 1));
    code.add(st.temp1, st.temp2);
    code.ldz(st.temp3.reversed(), ((index - 16) * 4) & 0x3F);
    code.add(st.temp1, st.temp3);
    code.ldz(st.temp3.reversed(), ((index - 7) * 4) & 0x3F);
    code.add(st.temp1, st.temp3);
    code.stz(st.temp1.reversed(), (index * 4) & 0x3F);
}

// Derives a state word for rounds 17..64 with an explicit round number.
static void gen_sha256_derive_state_word_2
    (Code &code, struct sha256_state &st, const Reg &round)
{
    Reg offset(st.temp3, 0, 1);

    // temp1 = state->w[(index - 15) & 0x0F];
    code.move(offset, round);
    code.sub(offset, 15 * 4);
    code.logand(offset, 0x3F);
    code.add(Reg::z_ptr(), offset);
    code.ldz(st.temp1.reversed(), 0);
    code.sub(Reg::z_ptr(), offset);

    // temp2 = state->w[(index - 2) & 0x0F];
    code.add(offset, 13 * 4);
    code.logand(offset, 0x3F);
    code.add(Reg::z_ptr(), offset);
    code.ldz(st.temp2.reversed(), 0);
    code.sub(Reg::z_ptr(), offset);

    // temp1 = state->w[index & 0x0F] =
    //   state->w[(index - 16) & 0x0F] + state->w[(index - 7) & 0x0F] +
    //   (rightRotate7(temp1) ^ rightRotate18(temp1) ^ (temp1 >> 3)) +
    //   (rightRotate17(temp2) ^ rightRotate19(temp2) ^ (temp2 >> 10));
    code.move(st.temp3, st.temp1);
    code.rol(st.temp3, 1); // 7 = 8 - 1
    code.move(st.temp4, st.temp1);
    code.ror(st.temp4, 2); // 18 = 16 + 2
    code.lsr(st.temp1, 3);
    code.logxor(st.temp1, st.temp3.shuffle(1, 2, 3, 0));
    code.logxor(st.temp1, st.temp4.shuffle(2, 3, 0, 1));
    code.move(st.temp3, st.temp2);
    code.ror(st.temp3, 1); // 17 = 16 + 1
    code.move(st.temp4, st.temp2);
    code.ror(st.temp4, 3); // 19 = 16 + 3
    code.lsr(st.temp2, 10);
    code.logxor(st.temp2, st.temp3.shuffle(2, 3, 0, 1));
    code.logxor(st.temp2, st.temp4.shuffle(2, 3, 0, 1));
    code.add(st.temp1, st.temp2);
    code.move(offset, round);
    code.sub(offset, 7 * 4);
    code.logand(offset, 0x3F);
    code.add(Reg::z_ptr(), offset);
    code.ldz(st.temp4.reversed(), 0);
    code.add(st.temp1, st.temp4);
    code.sub(Reg::z_ptr(), offset);
    code.move(offset, round);
    code.logand(offset, 0x3F);
    code.add(Reg::z_ptr(), offset);
    code.ldz(st.temp4.reversed(), 0);
    code.add(st.temp1, st.temp4);
    code.stz(st.temp1.reversed(), 0);
}

// Fully-unrolled version of the SHA256 transform function.
static void gen_sha256_transform_fully_unrolled(Code &code)
{
    struct sha256_state st;
    int index;

    // Set up the function prologue with 24 bytes of local variable storage.
    // Z points to the SHA256 state on input and output.
    code.prologue_permutation("sha256_transform", 24);

    // Load the state into registers and the stack in preparation.
    gen_sha256_load(code, st, 24);

    // Unroll all rounds, expanding the "w" state array on the fly.
    // The state array is in big endian byte order which we take care
    // of transparently during the loads and stores to "w".
    for (index = 0; index < 64; ++index) {
        // Load or derive the next word from the "w" state array.
        if (index < 16)
            code.ldz(st.temp1.reversed(), index * 4);
        else
            gen_sha256_derive_state_word(code, st, index);

        // Compute the temp1 and temp2 values for this round.
        code.move(st.temp3, k[index]);
        gen_sha256_step(code, st);

        // Rotate the hash state, keeping "a" and "e" in registers.
        gen_sha256_rotate(code, st);
    }

    // Store the result back to the state.
    gen_sha256_store(code, st);
}

// Get the round constant table for SHA256 as a S-box.
static Sbox get_sha256_rc_table()
{
    unsigned char rc[256];
    for (int index = 0; index < 64; ++index) {
        rc[index * 4]     = (unsigned char)(k[index] >> 24);
        rc[index * 4 + 1] = (unsigned char)(k[index] >> 16);
        rc[index * 4 + 2] = (unsigned char)(k[index] >> 8);
        rc[index * 4 + 3] = (unsigned char)(k[index]);
    }
    return Sbox(rc, sizeof(rc));
}

// Partially unrolled version of the SHA256 transform function.
static void gen_sha256_transform_partially_unrolled(Code &code)
{
    struct sha256_state st;
    int index;

    // Set up the function prologue with 32 bytes of local variable storage.
    // Z points to the SHA256 state on input and output.
    code.prologue_permutation("sha256_transform", 32);
    code.usedX();

    // Load the state into registers and the stack in preparation.
    gen_sha256_load(code, st, 32);

    // Copy the Z pointer to X because we need Z for the round constant table.
    code.move(Reg::x_ptr(), Reg::z_ptr());
    code.sbox_setup(0, get_sha256_rc_table(), Reg(st.temp3, 0, 1));

    // Unroll all rounds 16 at a time.
    unsigned char derive_label = 0;
    unsigned char round_label = 0;
    unsigned char end_label = 0;
    for (index = 0; index < 64; index += 16) {
        // Derive the next 16 state word values if necessary.
        if (index > 0) {
            code.sbox_cleanup();
            code.move(Reg::z_ptr(), Reg::x_ptr());
            code.call(derive_label);
            code.sbox_setup(0, get_sha256_rc_table(), Reg(st.temp3, 0, 1));
            code.sbox_adjust_by_offset(index * 4);
        }

        // Perform the next 16 rounds in two groups of 8.
        code.call(round_label);
        code.call(round_label);

        // Reset the X pointer back to the start of the state words.
        code.sub_ptr_x(64);
    }
    code.jmp(end_label);

    // Perform 8 rounds in a subroutine.
    code.label(round_label);
    for (index = 0; index < 8; ++index) {
        // Load the next state word from the X pointer.
        code.ldx(st.temp1.reversed(), POST_INC);

        // Compute the temp1 and temp2 values for this round.
        code.sbox_load_inc(st.temp3.reversed());
        gen_sha256_step(code, st);

        // Rotate the hash state, keeping "a" and "e" in registers.
        gen_sha256_rotate(code, st);
    }
    code.ret();

    // Derive the state words for the next 16 rounds.
    code.label(derive_label);
    for (index = 16; index < 32; ++index)
        gen_sha256_derive_state_word(code, st, index);
    code.ret();

    // Store the result back to the state.
    code.label(end_label);
    code.sbox_cleanup();
    code.move(Reg::z_ptr(), Reg::x_ptr());
    gen_sha256_store(code, st);
}

// Small version of the SHA256 transform function.
static void gen_sha256_transform_small(Code &code)
{
    struct sha256_state st;

    // Set up the function prologue with 34 bytes of local variable storage.
    // Z points to the SHA256 state on input and output.
    code.prologue_permutation("sha256_transform", 34);

    // Allocate a high register for the round counter.
    Reg round = code.allocateHighReg(1);

    // Load the state into registers and the stack in preparation.
    gen_sha256_load(code, st, 32);

    // Store Z into the stack frame.  We will be constantly switching back
    // and forth between Z as a state pointer and Z as a pointer to the
    // round constant table.  We need somewhere to get the original Z from.
    code.stlocal(Reg::z_ptr(), 32);

    // Top of the round loop.
    unsigned char top_label1 = 0;
    unsigned char top_label2 = 0;
    unsigned char top_label3 = 0;
    unsigned char end_label = 0;
    Reg offset = Reg(st.temp3, 0, 1);
    code.move(round, 0);
    code.label(top_label1);

    // Top of the round loop for rounds 1..16.
    // temp1 = w[round]
    code.move(offset, round);
    code.logand(offset, 0x3F);
    code.add(Reg::z_ptr(), offset);
    code.ldz(st.temp1.reversed(), 0);
    code.jmp(top_label3);

    // Top of the round loop for rounds 17..64.
    code.label(top_label2);

    // Generate the next key schedule word for rounds 17..64.
    gen_sha256_derive_state_word_2(code, st, round);

    // temp3 = rc[round]
    code.label(top_label3);
    code.sbox_setup2(0, get_sha256_rc_table(), round, Reg(st.temp3, 0, 1));
    code.sbox_load_inc(st.temp3.reversed());
    code.sbox_cleanup();

    // Perform the round step.
    gen_sha256_step(code, st);

    // Rotate the hash state, keeping "a" and "e" in registers.
    gen_sha256_rotate_full(code, st);

    // Restore the value of the Z pointer for the next round.
    code.ldlocal(Reg::z_ptr(), 32);

    // Bottom of the round loop.  For rounds 1..16 we branch back to
    // top_label1.  For rounds 17..64 we branch back to top_label2.
    code.add(round, 4);
    code.breq(end_label);
    code.compare(round, 16 * 4);
    code.brcs(top_label1);
    code.jmp(top_label2);

    // Store the result back to the state.
    code.label(end_label);
    gen_sha256_store(code, st);
}

static bool test_sha256_transform(Code &code, const gencrypto::TestVector &vec)
{
    unsigned char state[96];
    if (!vec.populate(state, 32, "Hash_In"))
        return false;
    if (!vec.populate(state + 32, 64, "Data"))
        return false;
    code.exec_permutation(state, 96);
    return vec.check(state, 32, "Hash_Out");
}

static void gen_sha256_rc_table(Code &code)
{
    code.sbox_add(0, get_sha256_rc_table());
}

GENCRYPTO_REGISTER_AVR("sha256_transform", "full", "avr5",
                       gen_sha256_transform_fully_unrolled,
                       test_sha256_transform);
GENCRYPTO_REGISTER_AVR("sha256_transform", "partial", "avr5",
                       gen_sha256_transform_partially_unrolled,
                       test_sha256_transform);
GENCRYPTO_REGISTER_AVR("sha256_transform", "small", "avr5",
                       gen_sha256_transform_small,
                       test_sha256_transform);
GENCRYPTO_REGISTER_AVR("sha256_rc_table", 0, "avr5",
                       gen_sha256_rc_table, 0);
