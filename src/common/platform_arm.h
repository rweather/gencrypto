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

#ifndef GENCRYPTO_PLATFORM_ARM_H
#define GENCRYPTO_PLATFORM_ARM_H

#include "platform.h"

namespace gencrypto
{

/**
 * \brief Common platform information for ARM platforms.
 */
class Platform_ARM : public Platform
{
public:
    /**
     * \brief Constructs an ARM platform handler.
     */
    Platform_ARM();

    /**
     * \brief Destroys this ARM platform handler.
     */
    virtual ~Platform_ARM();

    // Override parent class methods.
    BasicRegister::Size nativeWordSize() const;
    void unary
        (CodeGenerator *code, Insn::Type type,
         const SizedRegister &dest, const SizedRegister &src) const;
    void binary
        (CodeGenerator *code, Insn::Type type, const SizedRegister &dest,
         const SizedRegister &src1, const SizedRegister &src2, bool setc) const;
    void binary
        (CodeGenerator *code, Insn::Type type, const SizedRegister &dest,
         const SizedRegister &src1, const SizedRegister &src2,
         Insn::Modifier modifier, uint8_t shift, bool setc) const;
    void binaryImm
        (CodeGenerator *code, Insn::Type type, const SizedRegister &dest,
         const SizedRegister &src1, Insn::ImmValue immValue, bool setc) const;
    void writeInsn
        (const CodeGenerator *code, std::ostream &out,
         const Insn &insn) const;
};

/**
 * \brief Platform information for ARMv6 platforms.
 */
class Platform_ARMv6 : public Platform_ARM
{
public:
    /**
     * \brief Constructs an ARMv6 platform handler.
     */
    Platform_ARMv6();

    /**
     * \brief Destroys this ARMv6 platform handler.
     */
    virtual ~Platform_ARMv6();

    // Override parent class methods.
    bool validateImmediate
        (Insn::Type type, Insn::ImmValue value,
         BasicRegister::Size size) const;
    void moveImm
        (CodeGenerator *code, const SizedRegister &reg,
         Insn::ImmValue value) const;

protected:
    /**
     * \brief Constructs an ARMv6 platform handler that simulates ARMv6m.
     *
     * Used by the Platform_ARMv6m_Simulated subclass only.
     *
     * The registers usage will be restricted to match ARMv6m.
     */
    Platform_ARMv6(bool simulatev6m);
};

/**
 * \brief Platform information for ARMv6m platforms.
 */
class Platform_ARMv6m : public Platform_ARM
{
public:
    /**
     * \brief Constructs an ARMv6m platform handler.
     */
    Platform_ARMv6m();

    /**
     * \brief Destroys this ARMv6m platform handler.
     */
    virtual ~Platform_ARMv6m();

    // Override parent class methods.
    bool validateImmediate
        (Insn::Type type, Insn::ImmValue value,
         BasicRegister::Size size) const;
    void moveImm
        (CodeGenerator *code, const SizedRegister &reg,
         Insn::ImmValue value) const;
};

/**
 * \brief Platform information for ARMv6m platforms simulated on top of ARMv6.
 */
class Platform_ARMv6m_Simulated : public Platform_ARMv6
{
public:
    /**
     * \brief Constructs a simulated ARMv6m platform handler.
     */
    Platform_ARMv6m_Simulated();

    /**
     * \brief Destroys this simulated ARMv6m platform handler.
     */
    virtual ~Platform_ARMv6m_Simulated();

    // Override parent class methods.
    bool validateImmediate
        (Insn::Type type, Insn::ImmValue value,
         BasicRegister::Size size) const;
    void moveImm
        (CodeGenerator *code, const SizedRegister &reg,
         Insn::ImmValue value) const;
};

/**
 * \brief Platform information for ARMv7m platforms.
 */
class Platform_ARMv7m : public Platform_ARM
{
public:
    /**
     * \brief Constructs an ARMv7m platform handler.
     */
    Platform_ARMv7m();

    /**
     * \brief Destroys this ARMv7m platform handler.
     */
    virtual ~Platform_ARMv7m();

    // Override parent class methods.
    bool validateImmediate
        (Insn::Type type, Insn::ImmValue value,
         BasicRegister::Size size) const;
    void moveImm
        (CodeGenerator *code, const SizedRegister &reg,
         Insn::ImmValue value) const;
};

/**
 * \brief Platform information for 64-bit ARMv8a platforms.
 */
class Platform_ARMv8a : public Platform_ARM
{
public:
    /**
     * \brief Constructs an ARMv8a platform handler.
     */
    Platform_ARMv8a();

    /**
     * \brief Destroys this ARMv8a platform handler.
     */
    virtual ~Platform_ARMv8a();

    // Override parent class methods.
    BasicRegister::Size nativeWordSize() const;

    // Override parent class methods.
    bool validateImmediate
        (Insn::Type type, Insn::ImmValue value,
         BasicRegister::Size size) const;
    void moveImm
        (CodeGenerator *code, const SizedRegister &reg,
         Insn::ImmValue value) const;
};

} // namespace gencrypto

#endif
