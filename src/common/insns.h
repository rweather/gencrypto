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

#ifndef GENCRYPTO_INSNS_H
#define GENCRYPTO_INSNS_H

#include "regs.h"

namespace gencrypto
{

/**
 * \brief Information about a generic instruction.
 */
class Insn
{
public:
    /**
     * \brief Immediate value to supply to an instruction.
     */
    typedef uint64_t ImmValue;

    /**
     * \brief Label index for a branch or label instruction.
     */
    typedef uint16_t Label;

    /**
     * \brief Type of instruction.
     */
    enum Type
    {
        Unknown,    /**< No instruction type assigned yet */
        ADC,        /**< Add with carry */
        ADCI,       /**< Add immediate to register with carry */
        ADD,        /**< Add without carry */
        ADDI,       /**< Add immediate to register without carry */
        AND,        /**< Logical AND */
        ANDI,       /**< Logical AND with immediate */
        ASR,        /**< Arithmetic shift right */
        ASRI,       /**< Arithmetic shift right by immediate shift count */
        BIC,        /**< Logical bit clear: x = (~y) & z */
        BICI,       /**< Logical bit clear with immediate */
        BREQ,       /**< Branch if equal */
        BRGES,      /**< Branch if greater than or equal (signed) */
        BRGEU,      /**< Branch if greater than or equal (unsigned) */
        BRGTS,      /**< Branch if greater than (signed) */
        BRGTU,      /**< Branch if greater than (unsigned) */
        BRLES,      /**< Branch if less than or equal (signed) */
        BRLEU,      /**< Branch if less than or equal (unsigned) */
        BRLTS,      /**< Branch if less than (signed) */
        BRLTU,      /**< Branch if less than (unsigned) */
        BRNE,       /**< Branch if not equal */
        CMP,        /**< Compare two registers */
        CMPI,       /**< Compare register with immediate */
        CMPNI,      /**< Compare register with negated immediate */
        CMP_BREQ,   /**< Compare registers and branch if equal */
        CMP_BRNE,   /**< Compare registers and branch if not equal */
        CMPI_BREQ,  /**< Compare with immediate and branch if equal */
        CMPI_BRNE,  /**< Compare with immediate and branch if not equal */
        EXTS,       /**< Sign-extend of a register */
        EXTU,       /**< Unsigned-extend of a register */
        FSLI,       /**< Funnel shift left by immediate shift count */
        FSRI,       /**< Funnel shift right by immediate shift count */
        JMP,        /**< Unconditional jump */
        LABEL,      /**< Outputs a branch label at this point */
        LD8,        /**< Load unsigned 8-bit register from memory: x = [y + offset] */
        LD8S,       /**< Load signed 8-bit register from memory: x = [y + offset] */
        LD8_ARRAY,  /**< Load unsigned 8-bit register from an array: x = [y + z << shift] */
        LD8S_ARRAY, /**< Load signed 8-bit register from an array: x = [y + z << shift] */
        LD16,       /**< Load unsigned 16-bit register from memory: x = [y + offset] */
        LD16S,      /**< Load signed 16-bit register from memory: x = [y + offset] */
        LD16_ARRAY, /**< Load unsigned 16-bit register from an array: x = [y + z << shift] */
        LD16S_ARRAY,/**< Load signed 16-bit register from an array: x = [y + z << shift] */
        LD32,       /**< Load unsigned 32-bit register from memory: x = [y + offset] */
        LD32S,      /**< Load signed 32-bit register from memory: x = [y + offset] */
        LD32_ARRAY, /**< Load unsigned 32-bit register from an array: x = [y + z << shift] */
        LD32S_ARRAY,/**< Load signed 32-bit register from an array: x = [y + z << shift] */
        LD64,       /**< Load 64-bit register from memory: x = [y + offset] */
        LD64_ARRAY, /**< Load 64-bit register from an array: x = [y + z << shift] */
        LD_LABEL,   /**< Loads the address of a label into a register */
        LDARG8,     /**< Loads an 8-bit argument in the stack frame above the stacked return address */
        LDARG16,    /**< Loads a 16-bit argument in the stack frame above the stacked return address */
        LDARG32,    /**< Loads a 32-bit argument in the stack frame above the stacked return address */
        LDARG64,    /**< Loads a 64-bit argument in the stack frame above the stacked return address */
        LDI,        /**< Load arbitrary immediate value into register */
        LSL,        /**< Logical shift left */
        LSLI,       /**< Logical shift left by immediate shift count */
        LSR,        /**< Logical shift right */
        LSRI,       /**< Logical shift right by immediate shift count */
        MOV,        /**< Move register */
        MOVI,       /**< Move immediate value into register */
        MOVN,       /**< Move a NOT'ed immediate value into a register */
        MOVW,       /**< Moves an immediate value into the low 16-bits */
        MOVT,       /**< Moves an immediate value into the high 16-bits */
        NEG,        /**< Negate */
        NOP,        /**< No operation */
        NOT,        /**< Logical NOT */
        OR,         /**< Logical OR */
        ORI,        /**< Logical OR with immediate */
        POP,        /**< Pop register from stack */
        PUSH,       /**< Push register onto stack */
        PRINT,      /**< Print a hex word for diagnostic purposes */
        PRINTCH,    /**< Print a character for diagnostic purposes */
        PRINTLN,    /**< Print an end of line for diagnostic purposes */
        ROL,        /**< Rotate left */
        ROLI,       /**< Rotate left by immediate shift count */
        ROR,        /**< Rotate right */
        RORI,       /**< Rotate right by immediate shift count */
        SBC,        /**< Subtract with carry */
        SBCI,       /**< Subtract immediate with carry */
        SUB,        /**< Subtract without carry */
        SUBI,       /**< Subtract immediate without carry */
        SUBR,       /**< Reverse subtract without carry */
        SUBRI,      /**< Reverse subtract immediate without carry */
        ST8,        /**< Store 8-bit register to memory: [y + offset] = x */
        ST8_ARRAY,  /**< Store 8-bit register to an array: [y + z << shift] = x */
        ST16,       /**< Store 16-bit register to memory: [y + offset] = x */
        ST16_ARRAY, /**< Store 16-bit register to an array: [y + z << shift] = x */
        ST32,       /**< Store 32-bit register to memory: [y + offset] = x */
        ST32_ARRAY, /**< Store 32-bit register to an array: [y + z << shift] = x */
        ST64,       /**< Store 64-bit register to memory: [y + offset] = x */
        ST64_ARRAY, /**< Store 64-bit register to an array: [y + z << shift] = x */
        SWAP,       /**< Swap the bytes in a register */
        XOR,        /**< Exclusive-OR */
        XORI        /**< Exclusive-OR with immediate */
    };

    /**
     * \brief Modifier for ARM-style "shift and operate" instructions.
     */
    enum Modifier
    {
        MOD_NONE,   /**< No modifier */
        MOD_ASR,    /**< Arithmetic shift right modifier */
        MOD_LSL,    /**< Left shift modifier */
        MOD_LSR,    /**< Right shift modifier */
        MOD_ROR     /**< Right rotate modifier */
    };

    /**
     * \brief Options that may be added to an instruction to affect
     * how the backend generates the assembly code.
     */
    enum Option
    {
        NONE,       /**< No options */
        SHORT,      /**< Short instruction; e.g. thumb instructions on ARM */
        SETC        /**< Set condition codes */
    };

    /**
     * \brief Flags that indicate which fields are in the instruction.
     */
    enum Fields
    {
        DEST = 1,   /**< Destination register is set */
        SRC1 = 2,   /**< First source register is set */
        SRC2 = 4,   /**< Second source register is set */
        IMM  = 8,   /**< Immediate value is set */
        LAB  = 16,  /**< Label reference is set */
    };

    /**
     * \brief Constructs an empty instruction (reads as NOP).
     */
    Insn() : p(0) {}

    /**
     * \brief Constructs a copy of another instruction.
     *
     * \param other The other instruction to copy.
     */
    Insn(const Insn &other);

    /**
     * \brief Destroys this instruction.
     */
    ~Insn();

    /**
     * \brief Assigns a copy of another instruction to this one.
     *
     * \param other The other instruction to copy.
     *
     * \return A reference to this instruction.
     */
    Insn &operator=(const Insn &other);

    /**
     * \brief Gets the type of this instruction.
     *
     * \return The instruction type.
     */
    Type type() const { return read()->type; }

    /**
     * \brief Sets the type of this instruction.
     *
     * \param type The instruction type.
     */
    void setType(Type type) { write()->type = type; }

    /**
     * \brief Determine if this instruction's type has not yet been set.
     *
     * \return Returns true if this instruction is null; false otherwise.
     */
    bool isNull() const { return type() == Unknown; }

    /**
     * \brief Gets the destination register for this instruction.
     *
     * \return The destination register.
     *
     * \note In the case of a store instruction, this is the source
     * and src1() specifies the base register for the memory location.
     */
    SizedRegister dest() const { return read()->dest; }

    /**
     * \brief Sets the destination register for this instruction.
     *
     * \param dest The destination register.
     */
    void setDest(const SizedRegister &dest)
        { write()->dest = dest; write()->fields |= DEST; }

    /**
     * \brief Determine if this instruction has a destination register set.
     *
     * \return Returns true if a destination register is set.
     */
    bool hasDest() const { return (read()->fields & DEST) != 0; }

    /**
     * \brief Gets the first source register for this instruction.
     *
     * \return The first source register.
     */
    SizedRegister src1() const { return read()->src1; }

    /**
     * \brief Sets the first source register for this instruction.
     *
     * \param src1 The first source register.
     */
    void setSrc1(const SizedRegister &src1)
        { write()->src1 = src1; write()->fields |= SRC1; }

    /**
     * \brief Determine if this instruction has a first source register set.
     *
     * \return Returns true if a first source register is set.
     */
    bool hasSrc1() const { return (read()->fields & SRC1) != 0; }

    /**
     * \brief Gets the second source register for this instruction.
     *
     * \return The second source register.
     */
    SizedRegister src2() const { return read()->src2; }

    /**
     * \brief Sets the second source register for this instruction.
     *
     * \param src1 The second source register.
     */
    void setSrc2(const SizedRegister &src2)
        { write()->src2 = src2; write()->fields |= SRC2; }

    /**
     * \brief Determine if this instruction has a second source register set.
     *
     * \return Returns true if a second source register is set.
     */
    bool hasSrc2() const { return (read()->fields & SRC2) != 0; }

    /**
     * \brief Gets the shift modifier for this instruction.
     *
     * \return The shift modifier.
     *
     * \sa shift()
     */
    Modifier modifier() const { return read()->modifier; }

    /**
     * \brief Sets the shift modifier for this instruction.
     *
     * \param modifier The shift modifier.
     */
    void setModifier(Modifier modifier) { write()->modifier = modifier; }

    /**
     * \brief Gets the shift amount for this instruction.
     *
     * \return The shift amount.
     */
    uint8_t shift() const { return read()->shift; }

    /**
     * \brief Sets the shift amount for this instruction.
     *
     * \param shift The shift amount.
     */
    void setShift(uint8_t shift) { write()->shift = shift; }

    /**
     * \brief Gets the immediate value for this instruction.
     *
     * \return The immediate value.
     */
    ImmValue immValue() const { return read()->immValue; }

    /**
     * \brief Sets the immediate value for this instruction.
     *
     * \param immValue The immediate value.
     */
    void setImmValue(ImmValue immValue)
        { write()->immValue = immValue; write()->fields |= IMM; }

    /**
     * \brief Determine if this instruction has an immediate value set.
     *
     * \return Returns true if an immediate value is set.
     */
    bool hasImmValue() const { return (read()->fields & IMM) != 0; }

    /**
     * \brief Gets the label for this branch instruction.
     *
     * \return The reference for the label.
     */
    Label label() const { return (Label)(read()->immValue); }

    /**
     * \brief Sets the label for this branch instruction.
     *
     * \param label The reference for the label.
     */
    void setLabel(Label label)
        { write()->immValue = label; write()->fields |= LAB; }

    /**
     * \brief Determine if this instruction has a label reference set.
     *
     * \return Returns true if a label reference is set.
     */
    bool hasLabel() const { return (read()->fields & LAB) != 0; }

    /**
     * \brief Gets the option information for this instruction.
     *
     * \return The option information.
     */
    Option option() const { return read()->option; }

    /**
     * \brief Sets the option information for this instruction.
     *
     * \param option The option information.
     */
    void setOption(Option option) { write()->option = option; }

    /**
     * \brief Re-schedules this instruction by an offset.
     *
     * \param offset The offset to apply, or zero for no offset.
     *
     * This can be used to give a hint to the code generator that this
     * instruction should be repositioned within the final output.
     */
    void reschedule(int8_t offset) { write()->reschedule = offset; }

    /**
     * \brief Constructs a bare instruction with no arguments.
     *
     * \param type The type of instruction.
     *
     * \return The instruction.
     */
    static Insn bare(Type type);

    /**
     * \brief Constructs a unary instruction.
     *
     * \param type The type of instruction.
     * \param dest The destination register for the result.
     * \param src The source value to operate on, which may be the
     * same as \a dest for in-place unary instructions.
     * \param option Extra options for the instruction.
     *
     * \return The instruction.
     */
    static Insn unary
        (Type type, const SizedRegister &dest, const SizedRegister &src,
         Option option = NONE);

    /**
     * \brief Constructs a binary instruction.
     *
     * \param type The type of instruction.
     * \param dest The destination register for the result.
     * \param src1 The first source value to operate on, which may be the
     * same as \a dest for in-place binary instructions.
     * \param src2 The second source value to operate on.
     * \param option Extra options for the instruction.
     *
     * \return The instruction.
     */
    static Insn binary
        (Type type, const SizedRegister &dest, const SizedRegister &src1,
         const SizedRegister &src2, Option option = NONE);

    /**
     * \brief Constructs a binary instruction where the second source
     * argument is modified by a shift amount.
     *
     * \param type The type of instruction.
     * \param dest The destination register for the result.
     * \param src1 The first source value to operate on, which may be the
     * same as \a dest for in-place binary instructions.
     * \param src2 The second source value to operate on.
     * \param modifier The modifier to apply such as MOD_ROR.
     * \param shift The shift amount to apply.
     * \param option Extra options for the instruction.
     *
     * If \a shift is zero, then the modifier will be ignored.
     *
     * \return The instruction.
     */
    static Insn binary
        (Type type, const SizedRegister &dest, const SizedRegister &src1,
         const SizedRegister &src2, Modifier modifier, uint8_t shift,
         Option option = NONE);

    /**
     * \brief Constructs a binary instruction with an immediate second argument.
     *
     * \param type The type of instruction.
     * \param dest The destination register for the result.
     * \param src1 The first source value to operate on, which may be the
     * same as \a dest for in-place binary instructions.
     * \param immValue The immediate value to use as the second argument.
     * \param option Extra options for the instruction.
     *
     * \return The instruction.
     */
    static Insn binaryImm
        (Type type, const SizedRegister &dest, const SizedRegister &src1,
         ImmValue immValue, Option option = NONE);

    /**
     * \brief Constructs an instrument that loads an immediate argument.
     *
     * \param type The type of instruction.
     * \param dest The destination register.
     * \param immValue The immediate value.
     * \param option Extra options for the instruction.
     *
     * \return The instruction.
     */
    static Insn moveImm
        (Type type, const SizedRegister &dest, ImmValue immValue,
         Option option = NONE);

    /**
     * \brief Constructs a branch instruction.
     *
     * \param type The type of instruction.
     * \param label The label reference.
     *
     * \return The instruction.
     */
    static Insn branch(Type type, Label label);

private:
    struct Private
    {
        uint32_t refCount;
        Type type;
        Modifier modifier;
        uint8_t shift;
        uint8_t fields;
        int8_t reschedule;
        Option option;
        SizedRegister dest;
        SizedRegister src1;
        SizedRegister src2;
        ImmValue immValue;

        Private();
        Private(const Private &other);
    };
    mutable Private *p;

    const Private *read() const;
    Private *write();
};

} // namespace gencrypto

#endif
