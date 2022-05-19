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
#include <cstdlib>
#include <ctime>

using namespace AVR;

// Adjustment to the Z pointer to access the high shares.
#define ASCON_OFFSET_ADJUST 64

// Locations of words in the state.  Offsets less than 64 are accessed
// relative to the Z pointer.  Offsets 64 or greater are accessed
// relative to the local stack space (Y pointer).
typedef struct
{
    // Operational location for x0, x1, x2, x3, x4, t0.
    int loc[6];

    // Original location in the state structure for x0, x1, x2, x3, x4.
    int st[5];

} ascon_locations_t;

// Load a single byte from the state.
static void load_byte
    (Code &code, const Reg &reg, int offset, int share, int byte)
{
    offset += share * 8;
    if (offset < 64)
        code.ldz(reg, offset + 7 - byte);       // Big endian order.
    else
        code.ldlocal(reg, offset + byte - 64);  // Little endian order.
}

// Store a single byte to the state.
static void store_byte
    (Code &code, const Reg &reg, int offset, int share, int byte)
{
    offset += share * 8;
    if (offset < 64)
        code.stz(reg, offset + 7 - byte);       // Big endian order.
    else
        code.stlocal(reg, offset + byte - 64);  // Little endian order.
}

// Load a 64-bit word from the state.
static void load_word
    (Code &code, const Reg &reg, int offset, int share)
{
    offset += share * 8;
    if (offset < 64)
        code.ldz(reg.reversed(), offset);       // Big endian order.
    else
        code.ldlocal(reg, offset - 64);         // Little endian order.
}

// Store a 64-bit word to the state.
static void store_word
    (Code &code, const Reg &reg, int offset, int share)
{
    offset += share * 8;
    if (offset < 64)
        code.stz(reg.reversed(), offset);       // Big endian order.
    else
        code.stlocal(reg, offset - 64);         // Little endian order.
}

// Compute "x ^= (~y) & z" using a 2-share masked representation.
static void bic_xor
    (Code &code, Reg &x_a, Reg &x_b, Reg &y_a, Reg &y_b, Reg &z_a, Reg &z_b)
{
    // x_a ^= (~y_a) & z_b;
    // x_a ^= (~y_a) & z_a;
    // x_b ^= y_b & z_b;
    // x_b ^= y_b & z_a;
    Reg t1 = code.allocateReg(1);
    Reg t2 = code.allocateReg(1);
    code.lognot(t1, y_a);
    code.move(t2, t1);
    code.logand(t1, z_a);
    code.logand(t2, z_b);
    code.logxor(x_a, t1);
    code.logxor(x_a, t2);
    code.move(t1, y_b);
    code.move(t2, y_b);
    code.logand(t1, z_a);
    code.logand(t2, z_b);
    code.logxor(x_b, t1);
    code.logxor(x_b, t2);
    code.releaseReg(t1);
    code.releaseReg(t2);
}

static void ascon_substitute
    (Code &code, const ascon_locations_t &locations, int offset, Reg &x2_a)
{
    // Allocate and load the registers for all byte shares.
    // The x2.a value has already been loaded by the calling function.
    Reg x0_a = code.allocateReg(1);
    Reg x1_a = code.allocateReg(1);
    Reg x3_a = code.allocateReg(1);
    Reg x4_a = code.allocateReg(1);
    Reg x0_b = code.allocateReg(1);
    Reg x1_b = code.allocateReg(1);
    Reg x2_b = code.allocateReg(1);
    Reg x3_b = code.allocateReg(1);
    Reg x4_b = code.allocateReg(1);
    load_byte(code, x0_a, locations.loc[0], 0, offset);
    load_byte(code, x0_b, locations.loc[0], 1, offset);
    load_byte(code, x1_a, locations.loc[1], 0, offset);
    load_byte(code, x1_b, locations.loc[1], 1, offset);
    load_byte(code, x2_b, locations.loc[2], 1, offset);
    load_byte(code, x3_a, locations.loc[3], 0, offset);
    load_byte(code, x3_b, locations.loc[3], 1, offset);
    load_byte(code, x4_a, locations.loc[4], 0, offset);
    load_byte(code, x4_b, locations.loc[4], 1, offset);

    // We need some temporary registers as well to hold the t0 shares.
    Reg t0_a = code.allocateReg(1);
    Reg t0_b = code.allocateReg(1);
    Reg t1_a = code.allocateReg(1);
    Reg t1_b = code.allocateReg(1);

    // Start of the substitution layer, first share.
    code.logxor(x0_a, x4_a);        // x0_a ^= x4_a;
    code.logxor(x4_a, x3_a);        // x4_a ^= x3_a;
    code.logxor(x2_a, x1_a);        // x2_a ^= x1_a;
    code.move(t1_a, x0_a);          // t1_a  = x0_a;

    // Start of the substitution layer, second share.
    code.logxor(x0_b, x4_b);        // x0_b ^= x4_b;
    code.logxor(x4_b, x3_b);        // x4_b ^= x3_b;
    code.logxor(x2_b, x1_b);        // x2_b ^= x1_b;
    code.move(t1_b, x0_b);          // t1_b  = x0_b;

    // Create zero as a pair of random shares, t0_b = t0_a.
    load_byte(code, t0_a, locations.loc[5], 0, offset);
    code.move(t0_b, t0_a);

    // Middle part of the substitution layer, Chi5.
    bic_xor(code, t0_a, t0_b, x0_a, x0_b, x1_a, x1_b);  // t0 ^= (~x0) & x1;
    bic_xor(code, x0_a, x0_b, x1_a, x1_b, x2_a, x2_b);  // x0 ^= (~x1) & x2;
    bic_xor(code, x1_a, x1_b, x2_a, x2_b, x3_a, x3_b);  // x1 ^= (~x2) & x3;
    bic_xor(code, x2_a, x2_b, x3_a, x3_b, x4_a, x4_b);  // x2 ^= (~x3) & x4;
    bic_xor(code, x3_a, x3_b, x4_a, x4_b, t1_a, t1_b);  // x3 ^= (~x4) & t1;
    code.logxor(x4_a, t0_a);        // x4_a ^= t0_a;
    code.logxor(x4_b, t0_b);        // x4_b ^= t0_b;

    // End of the substitution layer.
    code.logxor(x1_a, x0_a);        // x1_a ^= x0_a;
    code.logxor(x0_a, x4_a);        // x0_a ^= x4_a;
    code.logxor(x3_a, x2_a);        // x3_a ^= x2_a;
    code.lognot(x2_a);              // x2_a = ~x2_a;
    code.logxor(x1_b, x0_b);        // x1_b ^= x0_b;
    code.logxor(x0_b, x4_b);        // x0_b ^= x4_b;
    code.logxor(x3_b, x2_b);        // x3_b ^= x2_b;

    // Write all values back to the state except for x2_a which we
    // keep in registers between rounds.
    store_byte(code, x0_a, locations.loc[0], 0, offset);
    store_byte(code, x0_b, locations.loc[0], 1, offset);
    store_byte(code, x1_a, locations.loc[1], 0, offset);
    store_byte(code, x1_b, locations.loc[1], 1, offset);
    store_byte(code, x2_b, locations.loc[2], 1, offset);
    store_byte(code, x3_a, locations.loc[3], 0, offset);
    store_byte(code, x3_b, locations.loc[3], 1, offset);
    store_byte(code, x4_a, locations.loc[4], 0, offset);
    store_byte(code, x4_b, locations.loc[4], 1, offset);
    store_byte(code, t0_a, locations.loc[5], 0, offset);

    // Release all registers except x2_a.
    code.releaseReg(x0_a);
    code.releaseReg(x1_a);
    code.releaseReg(x3_a);
    code.releaseReg(x4_a);
    code.releaseReg(x0_b);
    code.releaseReg(x1_b);
    code.releaseReg(x2_b);
    code.releaseReg(x3_b);
    code.releaseReg(x4_b);
    code.releaseReg(t0_a);
    code.releaseReg(t0_b);
    code.releaseReg(t1_a);
    code.releaseReg(t1_b);
}

static void ascon_diffuse
    (Code &code, const ascon_locations_t &locations, const Reg &x,
     int word, int shift1, int shift2, int share = 0)
{
    // Compute "x ^= (x >>> shift1) ^ (x >>> shift2)".
    Reg t = code.allocateReg(8);
    if (word != 2 || share != 0) // x2_a is already in registers.
        load_word(code, x, locations.loc[word], share);
    code.move(t, x);
    code.ror(t, shift1);
    code.logxor(t, x);
    code.ror(x, shift2);
    code.logxor(x, t);
    if (word != 2 || share != 0)
        store_word(code, x, locations.loc[word], share);
    code.releaseReg(t);
}

static void gen_avr_ascon_x2_permutation(Code &code, int max_shares)
{
    // Set up the function prologue with 24 bytes of local variable storage.
    //
    // Z points to the permutation state on input and output.
    // X points to the preserved randomness on input.
    //
    // Local stack frame:
    //      16 bytes for a copy of the x4.a and x4.b shares.
    //      8 bytes for t0.a to hold the randomness from round to round.
    //
    // When the maximum number of shares is 3, we need 40 bytes instead:
    //
    //      16 bytes for a copy of the x3.a and x3.b shares.
    //      16 bytes for a copy of the x4.a and x4.b shares.
    //      8 bytes for t0.a to hold the randomness from round to round.
    Reg round = code.prologue_masked_permutation
        ("ascon_x2_permute", (max_shares == 2) ? 24 : 40);

    // We are short on registers, so allow r0 to be used as a temporary.
    code.setFlag(Code::TempR0);

    // Compute "round = ((0x0F - round) << 4) | round" to convert the
    // first round number into a round constant.
    Reg temp = code.allocateHighReg(1);
    code.move(temp, 0x0F);
    code.sub(temp, round);
    code.onereg(Insn::SWAP, temp.reg(0));
    code.logor(round, temp);
    code.releaseReg(temp);

    // Set up the locations of all words.  There are 80 bytes in the
    // state plus 8 bytes of preserved randomness.  We can efficiently
    // access only 64 bytes with the Z pointer without constant pointer
    // readjustment.  To alleviate this, we transfer some of the words
    // to the stack so that we can access them with respect to the Y
    // pointer instead.
    ascon_locations_t locations;
    if (max_shares == 2) {
        // When the maximum number of shares is 2, we can keep x0, x1, x2, x3
        // in their original location.  We transfer x4 and t0 to the stack.
        locations.st[0] = 0;
        locations.st[1] = 16;
        locations.st[2] = 32;
        locations.st[3] = 48;
        locations.st[4] = 64;
        locations.loc[0] = 0;
        locations.loc[1] = 16;
        locations.loc[2] = 32;
        locations.loc[3] = 48;
        locations.loc[4] = 64;
        locations.loc[5] = 80;
    } else {
        // When the maximum number of shares is 3, we can keep x0, x1, x2
        // in their original location.  We transfer x3, x4, t0 to the stack.
        locations.st[0] = 0;
        locations.st[1] = 24;
        locations.st[2] = 48;
        locations.st[3] = 72;
        locations.st[4] = 96;
        locations.loc[0] = 0;
        locations.loc[1] = 24;
        locations.loc[2] = 48;
        locations.loc[3] = 64;
        locations.loc[4] = 80;
        locations.loc[5] = 96;
    }

    // Transfer the preserved randomness from the caller to local t0.a.
    Reg x2 = code.allocateReg(8);
    code.ldx(x2.reversed(), POST_INC);
    store_word(code, x2, locations.loc[5], 0);

    // Release the X register for use as temporaries during the function.
    // We will reload it when it is time to pass t0.a back to the caller.
    code.setFlag(Code::TempX);

    // We keep x2.a in registers between rounds so preload it.
    load_word(code, x2, locations.st[2], 0);

    // Transfer x3 and x4 to the stack to make them easier to access.
    Reg t0 = code.allocateReg(8);
    code.add_ptr_z(ASCON_OFFSET_ADJUST);
    if (max_shares == 3) {
        load_word(code, t0, locations.st[3] - ASCON_OFFSET_ADJUST, 0);
        store_word(code, t0, locations.loc[3], 0);
        load_word(code, t0, locations.st[3] - ASCON_OFFSET_ADJUST, 1);
        store_word(code, t0, locations.loc[3], 1);
    }
    load_word(code, t0, locations.st[4] - ASCON_OFFSET_ADJUST, 0);
    store_word(code, t0, locations.loc[4], 0);
    load_word(code, t0, locations.st[4] - ASCON_OFFSET_ADJUST, 1);
    store_word(code, t0, locations.loc[4], 1);
    code.sub_ptr_z(ASCON_OFFSET_ADJUST);
    code.releaseReg(t0);

    // Top of the round loop.
    unsigned char top_label = 0;
    code.label(top_label);

    // XOR the round constant with the low byte of "x2".
    code.logxor(x2, round);

    // Perform the substitution layer byte by byte.
    for (int index = 0; index < 8; ++index) {
        Reg x2_byte(x2, index, 1);
        ascon_substitute(code, locations, index, x2_byte);
    }

    // Perform the linear diffusion layer on each of the state words.
    // "x2.a" is still in registers, but nothing else is.
    t0 = code.allocateReg(8);
    ascon_diffuse(code, locations, t0, 0, 19, 28, 1);   // Second share.
    ascon_diffuse(code, locations, t0, 1, 61, 39, 1);
    ascon_diffuse(code, locations, t0, 2,  1,  6, 1);
    ascon_diffuse(code, locations, t0, 3, 10, 17, 1);
    ascon_diffuse(code, locations, t0, 4,  7, 41, 1);

    ascon_diffuse(code, locations, t0, 0, 19, 28, 0);   // First share.
    ascon_diffuse(code, locations, t0, 1, 61, 39, 0);
    ascon_diffuse(code, locations, x2, 2,  1,  6, 0);
    ascon_diffuse(code, locations, t0, 3, 10, 17, 0);
    ascon_diffuse(code, locations, t0, 4,  7, 41, 0);

    // Rotate t0_a right by 13 bits to produce the preseved random value
    // for the next round.  Equivalent to rotate left by 3 and right by 16.
    load_word(code, t0, locations.loc[5], 0);
    code.rol(t0, 3);
    store_word(code, t0.shuffle(2, 3, 4, 5, 6, 7, 0, 1), locations.loc[5], 0);
    code.releaseReg(t0);

    // Bottom of the round loop.  Adjust the round constant and
    // check to see if we have reached the final round.
    code.sub(round, 0x0F);
    code.compare_and_loop(round, 0x3C, top_label);

    // Store the final version of x2.a back to state memory.
    store_word(code, x2, locations.st[2], 0);

    // Transfer x3 and x4 from local variables back to the state.
    code.add_ptr_z(ASCON_OFFSET_ADJUST);
    if (max_shares == 3) {
        load_word(code, x2, locations.loc[3], 0);
        store_word(code, x2, locations.st[3] - ASCON_OFFSET_ADJUST, 0);
        load_word(code, x2, locations.loc[3], 1);
        store_word(code, x2, locations.st[3] - ASCON_OFFSET_ADJUST, 1);
    }
    load_word(code, x2, locations.loc[4], 0);
    store_word(code, x2, locations.st[4] - ASCON_OFFSET_ADJUST, 0);
    load_word(code, x2, locations.loc[4], 1);
    store_word(code, x2, locations.st[4] - ASCON_OFFSET_ADJUST, 1);

    // Transfer the preserved randomness in t0.a back to the caller.
    code.load_output_ptr();
    load_word(code, x2, locations.loc[5], 0);
    code.stx(x2.reversed(), POST_INC);
}

/* Load a big-endian 64-bit word from a byte buffer */
#define be_load_word64(ptr) \
    ((((uint64_t)((ptr)[0])) << 56) | \
     (((uint64_t)((ptr)[1])) << 48) | \
     (((uint64_t)((ptr)[2])) << 40) | \
     (((uint64_t)((ptr)[3])) << 32) | \
     (((uint64_t)((ptr)[4])) << 24) | \
     (((uint64_t)((ptr)[5])) << 16) | \
     (((uint64_t)((ptr)[6])) << 8) | \
      ((uint64_t)((ptr)[7])))

/* Store a big-endian 64-bit word into a byte buffer */
#define be_store_word64(ptr, x) \
    do { \
        uint64_t _x = (x); \
        (ptr)[0] = (uint8_t)(_x >> 56); \
        (ptr)[1] = (uint8_t)(_x >> 48); \
        (ptr)[2] = (uint8_t)(_x >> 40); \
        (ptr)[3] = (uint8_t)(_x >> 32); \
        (ptr)[4] = (uint8_t)(_x >> 24); \
        (ptr)[5] = (uint8_t)(_x >> 16); \
        (ptr)[6] = (uint8_t)(_x >> 8); \
        (ptr)[7] = (uint8_t)_x; \
    } while (0)

// Get a random 64-bit word.
static uint64_t get_random(void)
{
    static bool initialized = false;
    if (!initialized) {
        srand(time(NULL));
        initialized = true;
    }
    // rand() produces a 31-bit number; we need a 64-bit number.
    return ((uint64_t)rand()) |
           (((uint64_t)rand()) << 31) |
           (((uint64_t)rand()) << 62);
}

// Mask the input state.
static void mask
    (unsigned char out[120], const unsigned char in[40], int max_shares)
{
    uint64_t word;
    uint64_t random;
    int index;
    memset(out, 0, 120);
    for (index = 0; index < 5; ++index) {
        random = get_random();
        word = be_load_word64(in + index * 8);
        word ^= random;
        be_store_word64(out + index * max_shares * 8, word);
        be_store_word64(out + index * max_shares * 8 + 8, random);
    }
}

// Unmask the output state.
static void unmask
    (unsigned char out[40], const unsigned char in[120], int max_shares)
{
    uint64_t word;
    uint64_t random;
    int index;
    for (index = 0; index < 5; ++index) {
        word = be_load_word64(in + index * max_shares * 8);
        random = be_load_word64(in + index * max_shares * 8 + 8);
        word ^= random;
        be_store_word64(out + index * 8, word);
    }
}

static bool test_avr_ascon_x2_permutation
    (Code &code, const gencrypto::TestVector &vec, int max_shares)
{
    int firstRound = vec.valueAsInt("First_Round", 0);
    unsigned char input[40];
    unsigned char output[40];
    unsigned char preserve[8];
    unsigned char state[120];
    if (firstRound < 0 || firstRound > 12)
        return false;
    if (!vec.populate(input, sizeof(input), "Input"))
        return false;
    mask(state, input, max_shares);
    be_store_word64(preserve, get_random());
    code.exec_masked_permutation(state, 120, firstRound, preserve, 8);
    unmask(output, state, max_shares);
    return vec.check(output, sizeof(output), "Output");
}

static void gen_avr_ascon_x2_permutation_2(Code &code)
{
    gen_avr_ascon_x2_permutation(code, 2);
}

static void gen_avr_ascon_x2_permutation_3(Code &code)
{
    gen_avr_ascon_x2_permutation(code, 3);
}

static bool test_avr_ascon_x2_permutation_2
    (Code &code, const gencrypto::TestVector &vec)
{
    return test_avr_ascon_x2_permutation(code, vec, 2);
}

static bool test_avr_ascon_x2_permutation_3
    (Code &code, const gencrypto::TestVector &vec)
{
    return test_avr_ascon_x2_permutation(code, vec, 3);
}

GENCRYPTO_REGISTER_AVR("ascon_x2_permute", "2shares", "avr5",
                       gen_avr_ascon_x2_permutation_2,
                       test_avr_ascon_x2_permutation_2);
GENCRYPTO_REGISTER_AVR("ascon_x2_permute", "3shares", "avr5",
                       gen_avr_ascon_x2_permutation_3,
                       test_avr_ascon_x2_permutation_3);
