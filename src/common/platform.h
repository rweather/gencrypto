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

#ifndef GENCRYPTO_PLATFORM_H
#define GENCRYPTO_PLATFORM_H

#include "regs.h"
#include "insns.h"
#include <ostream>

namespace gencrypto
{

class CodeGenerator;

/**
 * \brief Platform information.
 */
class Platform
{
    friend class CodeGenerator;
public:
    Platform();
    virtual ~Platform();

    /**
     * \brief Major features of the platform.
     */
    enum Feature
    {
        /** Two address instructions of the form "x op= y" are available */
        TwoAddress          = 0x0001,

        /** Three address instructions of the form "x = y op z" are available */
        ThreeAddress        = 0x0002,

        /** ARM instructions of the form "x = y op (z sop n)" are available */
        ShiftAndOperate     = 0x0004,

        /** Registers are split into two groups: "low vs high" or
         *  "data vs address" where only one group can be used for
         *  general-purpose arithmetic and logical instructions. */
        SplitRegisters      = 0x0008,

        /** Platform is register poor with very few allocatable registers */
        RegisterPoor        = 0x0010,

        /** Platform is register rich with lots of allocatable registers */
        RegisterRich        = 0x0020,

        /** Rotations must be implemented in terms of left/right shifts */
        ShiftToRotate       = 0x0040,

        /** Funnel shift instructions are available to implement rotations */
        FunnelShift         = 0x0080,

        /** Platform has a "bic" bit-clear instruction: x = (~y) & z */
        BitClear            = 0x0100,

        /** Platform is big-endian (defaults to little-endian if not set */
        BigEndian           = 0x0200,

        /** Unary two-address instructions can have a different destination.
         *  Without this, unary operations must be in-place */
        UnaryDest           = 0x0400,

        /** Platform has combined "compare and branch" instructions */
        CompareAndBranch    = 0x0800,
    };

    /**
     * \brief Determine if this platform has a specific feature.
     *
     * \param feature Identifier for the feature.
     *
     * \return Returns true if this platform has the feature; false if not.
     */
    bool hasFeature(Feature feature) const
        { return (m_features & feature) == feature; }

    /**
     * \brief Get the native word size of the platform.
     *
     * \return The native word size, usually Size32 or Size64.
     */
    virtual BasicRegister::Size nativeWordSize() const = 0;

    /**
     * \brief Get the address word size of the platform.
     *
     * \return The address word size, usually Size32 or Size64.  The default
     * is the same as nativeWordSize().
     */
    virtual BasicRegister::Size addressWordSize() const;

    /**
     * \brief Determine if the native word size is 8-bit for this platform.
     *
     * \return Returns true if the native word size is 8-bit; false otherwise.
     */
    bool is8Bit() const { return nativeWordSize() == BasicRegister::Size8; }

    /**
     * \brief Determine if the native word size is 16-bit for this platform.
     *
     * \return Returns true if the native word size is 16-bit; false otherwise.
     */
    bool is16Bit() const { return nativeWordSize() == BasicRegister::Size16; }

    /**
     * \brief Determine if the native word size is 32-bit for this platform.
     *
     * \return Returns true if the native word size is 32-bit; false otherwise.
     */
    bool is32Bit() const { return nativeWordSize() == BasicRegister::Size32; }

    /**
     * \brief Determine if the native word size is 64-bit for this platform.
     *
     * \return Returns true if the native word size is 64-bit; false otherwise.
     */
    bool is64Bit() const { return nativeWordSize() == BasicRegister::Size64; }

    /**
     * \brief Gets the basic register to use for the stack pointer.
     *
     * \return The stack pointer register.
     */
    BasicRegister stackPointer() const { return m_sp; }

    /**
     * \brief Gets a specific register by name.
     *
     * \param name The register name to search for.
     *
     * \return The named register or a null SizedRegister object if
     * the \a name does not exist.
     */
    SizedRegister registerForName(const std::string &name) const;

    /**
     * \brief Gets a specific register by number.
     *
     * \param number The register number to search for.
     *
     * \return The register or a null BasicRegister object if the
     * \a number does not exist.
     */
    BasicRegister registerForNumber(uint8_t number) const;

    /**
     * \brief Validates an immediate value for an instruction.
     *
     * \param type The instruction type.
     * \param value The value of the immediate.
     * \param size The size of the immediate value.
     *
     * \return Returns true if the value can be used with the specified
     * instruction, or false if the immediate needs to be loaded into a
     * register before it can be used.
     *
     * Platforms with a uniform instruction size have restrictions on
     * the immediate values that can be used.  Some instructions cannot
     * use immediate values at all.
     *
     * For example, ARMv6m can move a zero-extended 8-bit value into a
     * register with "MOV" but cannot use immediate values at all with
     * the negated "MVN" instruction.  ARMv7m has a wider range of
     * immediate values that can be used with both "MOV" and "MVN".
     */
    virtual bool validateImmediate
        (Insn::Type type, Insn::ImmValue value,
         BasicRegister::Size size) const = 0;

    /**
     * \brief Generates a unary instruction.
     *
     * \param code The code generator to add the instruction to.
     * \param type The type of instruction.
     * \param dest The destination register for the result.
     * \param src The source value to operate on, which may be the
     * same as \a dest for in-place unary instructions.
     */
    virtual void unary
        (CodeGenerator *code, Insn::Type type,
         const SizedRegister &dest, const SizedRegister &src) const = 0;

    /**
     * \brief Generates a binary instruction.
     *
     * \param code The code generator to add the instruction to.
     * \param type The type of instruction.
     * \param dest The destination register for the result.
     * \param src1 The first source value to operate on, which may be the
     * same as \a dest for in-place binary instructions.
     * \param src2 The second source value to operate on.
     * \param setc True if the condition codes should be set.
     */
    virtual void binary
        (CodeGenerator *code, Insn::Type type, const SizedRegister &dest,
         const SizedRegister &src1, const SizedRegister &src2,
         bool setc) const = 0;

    /**
     * \brief Generates a binary instruction where the second source
     * argument is modified by a shift amount.
     *
     * \param code The code generator to add the instruction to.
     * \param type The type of instruction.
     * \param dest The destination register for the result.
     * \param src1 The first source value to operate on, which may be the
     * same as \a dest for in-place binary instructions.
     * \param src2 The second source value to operate on.
     * \param modifier The modifier to apply such as MOD_ROR.
     * \param shift The shift amount to apply.
     * \param setc True if the condition codes should be set.
     *
     * If \a shift is zero, then the modifier will be ignored.
     */
    virtual void binary
        (CodeGenerator *code, Insn::Type type, const SizedRegister &dest,
         const SizedRegister &src1, const SizedRegister &src2,
         Insn::Modifier modifier, uint8_t shift, bool setc) const = 0;

    /**
     * \brief Generates a binary instruction with an immediate second argument.
     *
     * \param code The code generator to add the instruction to.
     * \param type The type of instruction.
     * \param dest The destination register for the result.
     * \param src1 The first source value to operate on, which may be the
     * same as \a dest for in-place binary instructions.
     * \param immValue The immediate value to use as the second argument.
     * \param setc True if the condition codes should be set.
     */
    virtual void binaryImm
        (CodeGenerator *code, Insn::Type type, const SizedRegister &dest,
         const SizedRegister &src1, Insn::ImmValue immValue,
         bool setc) const = 0;

    /**
     * \brief Generates instructions to load an arbitrary immediate
     * value into a register.
     *
     * \param code The code generator to add the instructions to.
     * \param reg The register to load the immediate value into.
     * \param value The value to load into the register.
     */
    virtual void moveImm
        (CodeGenerator *code, const SizedRegister &reg,
         Insn::ImmValue value) const = 0;

    /**
     * \brief Begins writing instructions for a function.
     *
     * This can be used to reset auxilliary state that is used by writeInsn().
     * Default implementation does nothing.
     */
    virtual void beginWrite() const;

    /**
     * \brief Writes an instruction to an assembly code output stream.
     *
     * \param code The code generator that contained the instruction,
     * for fetching other instruction properties if necessary.
     * \param out The output stream to write to.
     * \param insn The instruction to write.
     */
    virtual void writeInsn
        (const CodeGenerator *code, std::ostream &out,
         const Insn &insn) const = 0;

protected:

    /**
     * \brief Sets the flags that define the features of this platform.
     *
     * \param features The feature flags.
     */
    void setFeatures(uint32_t features) { m_features = features; }

    /**
     * \brief Adds a basic register for this platform.
     *
     * \param reg The basic register to add.
     */
    void addBasicRegister(const BasicRegister &reg)
        { m_registers.push_back(reg); }

    /**
     * \brief Adds a numbered register to the list of registers that can
     * be used for function arguments.
     *
     * \param number The register number.
     */
    void addArgumentRegister(uint8_t number)
        { m_arguments.push_back(registerForNumber(number)); }

    /**
     * \brief Sets the basic register to use for the stack pointer.
     *
     * \param sp The stack pointer register.
     */
    void setStackPointer(const BasicRegister &sp) { m_sp = sp; }

private:
    uint32_t m_features;
    std::vector<BasicRegister> m_registers;
    std::vector<BasicRegister> m_arguments;
    BasicRegister m_sp;
};

} // namespace gencrypto

#endif
