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

#include "platform_arm.h"
#include "codegen.h"

namespace gencrypto
{

Platform_ARM::Platform_ARM()
{
}

Platform_ARM::~Platform_ARM()
{
}

BasicRegister::Size Platform_ARM::nativeWordSize() const
{
    return BasicRegister::Size32;
}

static bool isLowReg(const SizedRegister &reg)
{
    return reg.number() < 8;
}

void Platform_ARM::unary
    (CodeGenerator *code, Insn::Type type,
     const SizedRegister &dest, const SizedRegister &src) const
{
    if (hasFeature(Platform::TwoAddress) && isLowReg(dest) && isLowReg(src)) {
        code->addInsn(Insn::unary(type, dest, src, Insn::SHORT));
    } else if (hasFeature(Platform::ThreeAddress)) {
        code->addInsn(Insn::unary(type, dest, src));
    } else {
        throw std::invalid_argument("invalid unary instruction");
    }
}

void Platform_ARM::binary
    (CodeGenerator *code, Insn::Type type, const SizedRegister &dest,
     const SizedRegister &src1, const SizedRegister &src2, bool setc) const
{
    if (hasFeature(Platform::TwoAddress) && dest == src1 &&
            isLowReg(dest) && isLowReg(src2)) {
        code->addInsn(Insn::binary(type, dest, src1, src2, Insn::SHORT));
    } else if (hasFeature(Platform::ThreeAddress)) {
        code->addInsn(Insn::binary(type, dest, src1, src2,
                                   setc ? Insn::SETC : Insn::NONE));
    } else {
        throw std::invalid_argument("invalid binary instruction");
    }
}

void Platform_ARM::binary
    (CodeGenerator *code, Insn::Type type, const SizedRegister &dest,
     const SizedRegister &src1, const SizedRegister &src2,
     Insn::Modifier modifier, uint8_t shift, bool setc) const
{
    if (hasFeature(Platform::TwoAddress) && dest == src1 &&
            isLowReg(dest) && isLowReg(src2) &&
            (modifier == Insn::MOD_NONE || shift == 0)) {
        code->addInsn(Insn::binary(type, dest, src1, src2, Insn::SHORT));
    } else if (hasFeature(Platform::ThreeAddress)) {
        if (modifier == Insn::MOD_NONE || shift == 0) {
            code->addInsn(Insn::binary(type, dest, src1, src2,
                                       setc ? Insn::SETC : Insn::NONE));
        } else {
            code->addInsn(Insn::binary(type, dest, src1, src2,
                                       modifier, shift,
                                       setc ? Insn::SETC : Insn::NONE));
        }
    } else {
        throw std::invalid_argument("invalid binary instruction with modifier");
    }
}

void Platform_ARM::binaryImm
    (CodeGenerator *code, Insn::Type type, const SizedRegister &dest,
     const SizedRegister &src1, Insn::ImmValue immValue, bool setc) const
{
    if (!validateImmediate(type, immValue, dest.size())) {
        throw std::invalid_argument
            ("invalid immediate for binary instruction");
    } else if (hasFeature(Platform::TwoAddress) &&
               dest == src1 && isLowReg(dest)) {
        code->addInsn(Insn::binaryImm(type, dest, src1, immValue, Insn::SHORT));
    } else if (hasFeature(Platform::ThreeAddress)) {
        code->addInsn(Insn::binaryImm(type, dest, src1, immValue,
                                      setc ? Insn::SETC : Insn::NONE));
    } else {
        throw std::invalid_argument
            ("invalid binary instruction with immediate");
    }
}

void Platform_ARM::writeInsn
    (const CodeGenerator *code, std::ostream &out, const Insn &insn) const
{
    // TODO
    (void)code;
    switch (insn.type()) {
    case Insn::Unknown: break;
    case Insn::ADC:
    case Insn::ADCI:
    case Insn::ADD:
    case Insn::ADDI:
    case Insn::AND:
    case Insn::ANDI:
    case Insn::ASR:
    case Insn::ASRI:
    case Insn::BIC:
    case Insn::BICI:
    case Insn::BREQ:
    case Insn::BRGES:
    case Insn::BRGEU:
    case Insn::BRGTS:
    case Insn::BRGTU:
    case Insn::BRLES:
    case Insn::BRLEU:
    case Insn::BRLTS:
    case Insn::BRLTU:
    case Insn::BRNE:
    case Insn::CMP:
    case Insn::CMPI:
    case Insn::CMPNI:
    case Insn::CMP_BREQ:
    case Insn::CMP_BRNE:
    case Insn::CMPI_BREQ:
    case Insn::CMPI_BRNE:
    case Insn::EXTS:
    case Insn::EXTU:
    case Insn::FSLI:
    case Insn::FSRI:
    case Insn::JMP:
    case Insn::LABEL:
    case Insn::LD8:
    case Insn::LD8S:
    case Insn::LD8_ARRAY:
    case Insn::LD8S_ARRAY:
    case Insn::LD16:
    case Insn::LD16S:
    case Insn::LD16_ARRAY:
    case Insn::LD16S_ARRAY:
    case Insn::LD32:
    case Insn::LD32S:
    case Insn::LD32_ARRAY:
    case Insn::LD32S_ARRAY:
    case Insn::LD64:
    case Insn::LD64_ARRAY:
    case Insn::LD_LABEL:
    case Insn::LDARG8:
    case Insn::LDARG16:
    case Insn::LDARG32:
    case Insn::LDARG64:
    case Insn::LDI:
    case Insn::LSL:
    case Insn::LSLI:
    case Insn::LSR:
    case Insn::LSRI:
    case Insn::MOV:
    case Insn::MOVI:
    case Insn::MOVN:
    case Insn::MOVW:
    case Insn::MOVT:
    case Insn::NEG:
    case Insn::NOP:
    case Insn::NOT:
    case Insn::OR:
    case Insn::ORI:
    case Insn::POP:
    case Insn::PUSH:
    case Insn::PRINT:
    case Insn::PRINTCH:
    case Insn::PRINTLN:
    case Insn::ROL:
    case Insn::ROLI:
    case Insn::ROR:
    case Insn::RORI:
    case Insn::SBC:
    case Insn::SBCI:
    case Insn::SUB:
    case Insn::SUBI:
    case Insn::SUBR:
    case Insn::SUBRI:
    case Insn::ST8:
    case Insn::ST8_ARRAY:
    case Insn::ST16:
    case Insn::ST16_ARRAY:
    case Insn::ST32:
    case Insn::ST32_ARRAY:
    case Insn::ST64:
    case Insn::ST64_ARRAY:
    case Insn::SWAP:
    case Insn::XOR:
    case Insn::XORI:
        break;
    }
}

Platform_ARMv6::Platform_ARMv6()
{
    // Specify the features for the platform.
    setFeatures(ThreeAddress | ShiftAndOperate | BitClear | UnaryDest);

    // Specify the registers for the platform.
    uint16_t nosave_flags =
        BasicRegister::ThreeAddress |
        BasicRegister::Address |
        BasicRegister::Data;
    uint16_t save_flags =
        BasicRegister::ThreeAddress |
        BasicRegister::Address |
        BasicRegister::Data |
        BasicRegister::CalleeSaved;
    uint16_t addr_only_flags =
        BasicRegister::ThreeAddress |
        BasicRegister::Address;
    uint16_t temp_flags = BasicRegister::Temporary;
    // Allocate argument registers r0..r3 in reverse order because
    // earlier registers are likely to be in use for actual arguments.
    addBasicRegister(BasicRegister::reg32(3, "r3", nosave_flags));
    addBasicRegister(BasicRegister::reg32(2, "r2", nosave_flags));
    addBasicRegister(BasicRegister::reg32(1, "r1", nosave_flags));
    addBasicRegister(BasicRegister::reg32(0, "r0", nosave_flags));
    addBasicRegister(BasicRegister::reg32(4, "r4", save_flags));
    addBasicRegister(BasicRegister::reg32(5, "r5", save_flags));
    addBasicRegister(BasicRegister::reg32(6, "r6", save_flags));
    addBasicRegister(BasicRegister::reg32(7, "r7", save_flags));
    addBasicRegister(BasicRegister::reg32(8, "r8", save_flags));
    addBasicRegister(BasicRegister::reg32(9, "r9", save_flags));
    addBasicRegister(BasicRegister::reg32(10, "r10", save_flags));
    addBasicRegister(BasicRegister::reg32(12, "ip", nosave_flags | temp_flags));
    addBasicRegister(BasicRegister::reg32(11, "fp", save_flags));
    addBasicRegister
        (BasicRegister::reg32
            (14, "lr", save_flags | BasicRegister::Link));
    BasicRegister sp =
        BasicRegister::reg32
            (13, "sp", addr_only_flags | BasicRegister::StackPointer
                                       | BasicRegister::NoAllocate);
    addBasicRegister(sp);
    setStackPointer(sp);
    addBasicRegister
        (BasicRegister::reg32
            (15, "pc", addr_only_flags | BasicRegister::ProgramCounter
                                       | BasicRegister::NoAllocate));

    // Specify the argument registers: r0..r3
    addArgumentRegister(0);
    addArgumentRegister(1);
    addArgumentRegister(2);
    addArgumentRegister(3);
}

Platform_ARMv6::Platform_ARMv6(bool simulatedv6m)
{
    // Specify the features for the platform.
    setFeatures(ThreeAddress | SplitRegisters | BitClear | UnaryDest);

    // Specify the registers for the platform.
    (void)simulatedv6m;
    uint16_t low_flags =
        BasicRegister::Address |
        BasicRegister::Data |
        BasicRegister::ThreeAddress;
    uint16_t high_flags =
        BasicRegister::Storage |
        BasicRegister::ThreeAddress;
    uint16_t save_flags =
        BasicRegister::CalleeSaved;
    uint16_t addr_only_flags =
        BasicRegister::Address |
        BasicRegister::ThreeAddress;
    uint16_t temp_flags = BasicRegister::Temporary;
    // Allocate argument registers r0..r3 in reverse order because
    // earlier registers are likely to be in use for actual arguments.
    addBasicRegister(BasicRegister::reg32(3, "r3", low_flags));
    addBasicRegister(BasicRegister::reg32(2, "r2", low_flags));
    addBasicRegister(BasicRegister::reg32(1, "r1", low_flags));
    addBasicRegister(BasicRegister::reg32(0, "r0", low_flags));
    addBasicRegister(BasicRegister::reg32(4, "r4", low_flags | save_flags));
    addBasicRegister(BasicRegister::reg32(5, "r5", low_flags | save_flags));
    addBasicRegister(BasicRegister::reg32(6, "r6", low_flags | save_flags));
    addBasicRegister(BasicRegister::reg32(7, "r7", low_flags | save_flags));
    addBasicRegister(BasicRegister::reg32(8, "r8", high_flags | save_flags));
    addBasicRegister(BasicRegister::reg32(9, "r9", high_flags | save_flags));
    addBasicRegister(BasicRegister::reg32(10, "r10", high_flags | save_flags));
    addBasicRegister(BasicRegister::reg32(12, "ip", high_flags | temp_flags));
    addBasicRegister(BasicRegister::reg32(11, "fp", high_flags | save_flags));
    addBasicRegister
        (BasicRegister::reg32
            (14, "lr", high_flags | save_flags | BasicRegister::Link));
    BasicRegister sp =
        BasicRegister::reg32
            (13, "sp", addr_only_flags | BasicRegister::StackPointer
                                       | BasicRegister::NoAllocate);
    addBasicRegister(sp);
    setStackPointer(sp);
    addBasicRegister
        (BasicRegister::reg32
            (15, "pc", addr_only_flags | BasicRegister::ProgramCounter
                                       | BasicRegister::NoAllocate));

    // Specify the argument registers: r0..r3
    addArgumentRegister(0);
    addArgumentRegister(1);
    addArgumentRegister(2);
    addArgumentRegister(3);
}

Platform_ARMv6::~Platform_ARMv6()
{
}

// Determine if a constant can be used as "Operand2" in an ARMv6 instruction.
static bool isOperand2ConstantARMv6(uint32_t value)
{
    int shift;

    // If the value is less than 256, then it can be used directly.
    if (value < 256U)
        return true;

    // Check if the value can be expressed as an 8-bit quantity that has
    // been rotated right by a multiple of 2 bits.  Rotate the value left
    // by 2 bits each time and check to see if it fits in 8 bits.
    for (shift = 2; shift <= 28; shift += 2) {
        value = (value << 2) | (value >> 30);
        if (value < 256U)
            return true;
    }

    // Not usable as a constant in "Operand2".
    return false;
}

bool Platform_ARMv6::validateImmediate
    (Insn::Type type, Insn::ImmValue value, BasicRegister::Size size) const
{
    (void)size;
    switch (type) {
    case Insn::ADCI:
    case Insn::ADDI:
    case Insn::ANDI:
    case Insn::BICI:
    case Insn::MOVI:
    case Insn::MOVN:
    case Insn::ORI:
    case Insn::SBCI:
    case Insn::SUBI:
    case Insn::SUBRI:
    case Insn::XORI:
        return isOperand2ConstantARMv6((uint32_t)value);
    case Insn::CMPI:
    case Insn::CMPNI:
        if (isOperand2ConstantARMv6((uint32_t)value))
            return true;
        else if (isOperand2ConstantARMv6((uint32_t)(-value)))
            return true;
        break;
    case Insn::ASRI:
    case Insn::LSLI:
    case Insn::LSRI:
    case Insn::ROLI:
    case Insn::RORI:
        return value < 32;
    case Insn::LD8:
    case Insn::LD8S:
    case Insn::ST8:
    case Insn::LD16:
    case Insn::LD16S:
    case Insn::ST16:
    case Insn::LD32:
    case Insn::LD32S:
    case Insn::ST32: {
        int64_t offset = (int64_t)value;
        return offset >= -4095 && offset <= 4095; }
    default: break;
    }
    return false;
}

void Platform_ARMv6::moveImm
    (CodeGenerator *code, const SizedRegister &reg, Insn::ImmValue value) const
{
    uint32_t val = (uint32_t)value;
    if (isOperand2ConstantARMv6(val))
        code->addInsn(Insn::moveImm(Insn::MOVI, reg, val));
    else if (isOperand2ConstantARMv6(~val))
        code->addInsn(Insn::moveImm(Insn::MOVN, reg, ~val));
    else
        code->addInsn(Insn::moveImm(Insn::LDI, reg, val));
}

Platform_ARMv6m::Platform_ARMv6m()
{
    // Specify the features for the platform.
    setFeatures(TwoAddress | SplitRegisters | BitClear | UnaryDest);

    // Specify the registers for the platform.
    uint16_t low_flags =
        BasicRegister::Address |
        BasicRegister::Data |
        BasicRegister::TwoAddress;
    uint16_t high_flags =
        BasicRegister::Storage;
    uint16_t save_flags =
        BasicRegister::CalleeSaved;
    uint16_t addr_only_flags =
        BasicRegister::Address;
    uint16_t temp_flags = BasicRegister::Temporary;
    // Allocate argument registers r0..r3 in reverse order because
    // earlier registers are likely to be in use for actual arguments.
    addBasicRegister(BasicRegister::reg32(3, "r3", low_flags));
    addBasicRegister(BasicRegister::reg32(2, "r2", low_flags));
    addBasicRegister(BasicRegister::reg32(1, "r1", low_flags));
    addBasicRegister(BasicRegister::reg32(0, "r0", low_flags));
    addBasicRegister(BasicRegister::reg32(4, "r4", low_flags | save_flags));
    addBasicRegister(BasicRegister::reg32(5, "r5", low_flags | save_flags));
    addBasicRegister(BasicRegister::reg32(6, "r6", low_flags | save_flags));
    addBasicRegister(BasicRegister::reg32(7, "r7", low_flags | save_flags));
    addBasicRegister(BasicRegister::reg32(8, "r8", high_flags | save_flags));
    addBasicRegister(BasicRegister::reg32(9, "r9", high_flags | save_flags));
    addBasicRegister(BasicRegister::reg32(10, "r10", high_flags | save_flags));
    addBasicRegister(BasicRegister::reg32(12, "ip", high_flags | temp_flags));
    addBasicRegister(BasicRegister::reg32(11, "fp", high_flags | save_flags));
    addBasicRegister
        (BasicRegister::reg32
            (14, "lr", high_flags | save_flags | BasicRegister::Link));
    BasicRegister sp =
        BasicRegister::reg32
            (13, "sp", addr_only_flags | BasicRegister::StackPointer
                                       | BasicRegister::NoAllocate);
    addBasicRegister(sp);
    setStackPointer(sp);
    addBasicRegister
        (BasicRegister::reg32
            (15, "pc", addr_only_flags | BasicRegister::ProgramCounter
                                       | BasicRegister::NoAllocate));

    // Specify the argument registers: r0..r3
    addArgumentRegister(0);
    addArgumentRegister(1);
    addArgumentRegister(2);
    addArgumentRegister(3);
}

Platform_ARMv6m::~Platform_ARMv6m()
{
}

bool Platform_ARMv6m::validateImmediate
    (Insn::Type type, Insn::ImmValue value, BasicRegister::Size size) const
{
    (void)size;
    switch (type) {
    case Insn::ADDI:
    case Insn::CMPI:
    case Insn::MOVI:
    case Insn::SUBI:
        return value < 256;
    case Insn::ASRI:
    case Insn::LSLI:
    case Insn::LSRI:
        return value < 32;
    case Insn::SUBRI:
        return value == 0;
    case Insn::LD8:
    case Insn::ST8:
        return value <= 31;
    case Insn::LD16:
    case Insn::ST16:
        return (value & 2) == 0 && value <= 62;
    case Insn::LD32:
    case Insn::LD32S:
    case Insn::ST32:
        return (value & 3) == 0 && value <= 124;
    default: break;
    }
    return false;
}

void Platform_ARMv6m::moveImm
    (CodeGenerator *code, const SizedRegister &reg, Insn::ImmValue value) const
{
    uint32_t val = (uint32_t)value;
    if (val < 256U && reg.number() < 8)
        code->addInsn(Insn::moveImm(Insn::MOVI, reg, val, Insn::SHORT));
    else
        code->addInsn(Insn::moveImm(Insn::LDI, reg, val));
}

Platform_ARMv6m_Simulated::Platform_ARMv6m_Simulated()
    : Platform_ARMv6(true)
{
}

Platform_ARMv6m_Simulated::~Platform_ARMv6m_Simulated()
{
}

bool Platform_ARMv6m_Simulated::validateImmediate
    (Insn::Type type, Insn::ImmValue value, BasicRegister::Size size) const
{
    (void)size;
    switch (type) {
    case Insn::ADDI:
    case Insn::CMPI:
    case Insn::MOVI:
    case Insn::SUBI:
        return value < 256;
    case Insn::ASRI:
    case Insn::LSLI:
    case Insn::LSRI:
        return value < 32;
    case Insn::SUBRI:
        return value == 0;
    case Insn::LD8:
    case Insn::ST8:
        return value <= 31;
    case Insn::LD16:
    case Insn::ST16:
        return (value & 2) == 0 && value <= 62;
    case Insn::LD32:
    case Insn::LD32S:
    case Insn::ST32:
        return (value & 3) == 0 && value <= 124;
    default: break;
    }
    return false;
}

void Platform_ARMv6m_Simulated::moveImm
    (CodeGenerator *code, const SizedRegister &reg, Insn::ImmValue value) const
{
    uint32_t val = (uint32_t)value;
    if (val < 256U && reg.number() < 8)
        code->addInsn(Insn::moveImm(Insn::MOVI, reg, val));
    else
        code->addInsn(Insn::moveImm(Insn::LDI, reg, val));
}

Platform_ARMv7m::Platform_ARMv7m()
{
    // Specify the features for the platform.
    setFeatures(TwoAddress | ThreeAddress | ShiftAndOperate |
                 BitClear | UnaryDest);

    // Specify the registers for the platform.
    uint16_t thumb = BasicRegister::TwoAddress;
    uint16_t nosave_flags =
        BasicRegister::ThreeAddress |
        BasicRegister::Address |
        BasicRegister::Data;
    uint16_t save_flags =
        BasicRegister::ThreeAddress |
        BasicRegister::Address |
        BasicRegister::Data |
        BasicRegister::CalleeSaved;
    uint16_t addr_only_flags =
        BasicRegister::ThreeAddress |
        BasicRegister::Address;
    uint16_t temp_flags = BasicRegister::Temporary;
    // Allocate argument registers r0..r3 in reverse order because
    // earlier registers are likely to be in use for actual arguments.
    addBasicRegister(BasicRegister::reg32(3, "r3", nosave_flags | thumb));
    addBasicRegister(BasicRegister::reg32(2, "r2", nosave_flags | thumb));
    addBasicRegister(BasicRegister::reg32(1, "r1", nosave_flags | thumb));
    addBasicRegister(BasicRegister::reg32(0, "r0", nosave_flags | thumb));
    addBasicRegister(BasicRegister::reg32(4, "r4", save_flags | thumb));
    addBasicRegister(BasicRegister::reg32(5, "r5", save_flags | thumb));
    addBasicRegister(BasicRegister::reg32(6, "r6", save_flags | thumb));
    addBasicRegister(BasicRegister::reg32(7, "r7", save_flags | thumb));
    addBasicRegister(BasicRegister::reg32(8, "r8", save_flags));
    addBasicRegister(BasicRegister::reg32(9, "r9", save_flags));
    addBasicRegister(BasicRegister::reg32(10, "r10", save_flags));
    addBasicRegister(BasicRegister::reg32(12, "ip", nosave_flags | temp_flags));
    addBasicRegister(BasicRegister::reg32(11, "fp", save_flags));
    addBasicRegister
        (BasicRegister::reg32
            (14, "lr", save_flags | BasicRegister::Link));
    BasicRegister sp =
        BasicRegister::reg32
            (13, "sp", addr_only_flags | BasicRegister::StackPointer
                                       | BasicRegister::NoAllocate);
    addBasicRegister(sp);
    setStackPointer(sp);
    addBasicRegister
        (BasicRegister::reg32
            (15, "pc", addr_only_flags | BasicRegister::ProgramCounter
                                       | BasicRegister::NoAllocate));

    // Specify the argument registers: r0..r3
    addArgumentRegister(0);
    addArgumentRegister(1);
    addArgumentRegister(2);
    addArgumentRegister(3);
}

Platform_ARMv7m::~Platform_ARMv7m()
{
}

// Determine if a constant can be used as "Operand2" in an ARMv7m instruction.
static bool isOperand2ConstantARMv7m(uint32_t value)
{
    int shift;
    uint32_t mask;

    // If the value is less than 256, then it can be used directly.
    if (value < 256U)
        return true;

    // If the value has the form 00XY00XY, XY00XY00, or XYXYXYXY, then
    // it can be used as a "modified immediate" in Thumb code.
    if ((value & 0x00FF00FFU) == value && (value >> 16) == (value & 0xFFU))
        return true;
    if ((value & 0xFF00FF00U) == value && (value >> 16) == (value & 0xFF00U))
        return true;
    if (((value >> 24) & 0xFF) == (value & 0xFF) &&
             ((value >> 16) & 0xFF) == (value & 0xFF) &&
             ((value >>  8) & 0xFF) == (value & 0xFF))
        return true;

    // Check if the value can be expressed as an 8-bit quantity that has
    // been rotated right by a multiple of 4 bits and the top-most bit
    // of the 8 is set to 1.
    for (shift = 0; shift <= 24; shift += 4) {
        mask = 0xFF000000U >> shift;
        if ((value & mask) != value)
            continue;
        mask = 0x80000000U >> shift;
        if ((value & mask) == mask)
            return true;
    }

    // Not usable as a constant in "Operand2".
    return false;
}

bool Platform_ARMv7m::validateImmediate
    (Insn::Type type, Insn::ImmValue value, BasicRegister::Size size) const
{
    (void)size;
    switch (type) {
    case Insn::ADCI:
    case Insn::ADDI:
    case Insn::ANDI:
    case Insn::BICI:
    case Insn::MOVI:
    case Insn::MOVN:
    case Insn::ORI:
    case Insn::SBCI:
    case Insn::SUBI:
    case Insn::SUBRI:
    case Insn::XORI:
        return isOperand2ConstantARMv7m((uint32_t)value);
    case Insn::CMPI:
    case Insn::CMPNI:
        if (isOperand2ConstantARMv7m((uint32_t)value))
            return true;
        else if (isOperand2ConstantARMv7m((uint32_t)(-value)))
            return true;
        break;
    case Insn::ASRI:
    case Insn::LSLI:
    case Insn::LSRI:
    case Insn::ROLI:
    case Insn::RORI:
        return value < 32;
    case Insn::LD8:
    case Insn::LD8S:
    case Insn::ST8:
    case Insn::LD16:
    case Insn::LD16S:
    case Insn::ST16:
    case Insn::LD32:
    case Insn::LD32S:
    case Insn::ST32: {
        int64_t offset = (int64_t)value;
        return offset >= -255 && offset <= 4095; }
    default: break;
    }
    return false;
}

void Platform_ARMv7m::moveImm
    (CodeGenerator *code, const SizedRegister &reg, Insn::ImmValue value) const
{
    uint32_t val = (uint32_t)value;
    if (val < 256U && reg.number() < 8) {
        code->addInsn(Insn::moveImm(Insn::MOVI, reg, val, Insn::SHORT));
    } else if (isOperand2ConstantARMv7m(val)) {
        code->addInsn(Insn::moveImm(Insn::MOVI, reg, val));
    } else if (isOperand2ConstantARMv7m(~val)) {
        code->addInsn(Insn::moveImm(Insn::MOVN, reg, ~val));
    } else {
        code->addInsn(Insn::moveImm(Insn::MOVW, reg, val & 0xFFFFU));
        if ((val & 0xFFFF0000U) != 0) {
            code->addInsn
                (Insn::binaryImm(Insn::MOVT, reg, reg, (val >> 16) & 0xFFFFU));
        }
    }
}

Platform_ARMv8a::Platform_ARMv8a()
{
    // Specify the features for the platform.
    setFeatures(ThreeAddress | ShiftAndOperate | RegisterRich |
                BitClear | UnaryDest);

    // Specify the registers for the platform.
    uint16_t nosave_flags =
        BasicRegister::ThreeAddress |
        BasicRegister::Address |
        BasicRegister::Data;
    uint16_t save_flags =
        BasicRegister::ThreeAddress |
        BasicRegister::Address |
        BasicRegister::Data |
        BasicRegister::CalleeSaved;
    uint16_t addr_only_flags =
        BasicRegister::ThreeAddress |
        BasicRegister::Address;
    uint16_t noalloc = BasicRegister::NoAllocate;
    // Put non-save and non-argument registers first in the allocation order.
    addBasicRegister(BasicRegister::reg3264(9, "w9", "x9", nosave_flags));
    addBasicRegister(BasicRegister::reg3264(10, "w10", "x10", nosave_flags));
    addBasicRegister(BasicRegister::reg3264(11, "w11", "x11", nosave_flags));
    addBasicRegister(BasicRegister::reg3264(12, "w12", "x12", nosave_flags));
    addBasicRegister(BasicRegister::reg3264(13, "w13", "x13", nosave_flags));
    addBasicRegister(BasicRegister::reg3264(14, "w14", "x14", nosave_flags));
    addBasicRegister(BasicRegister::reg3264(15, "w15", "x15", nosave_flags));
    // Allocate argument registers x0..x8 in reverse order because
    // earlier registers are likely to be in use for actual arguments.
    // Note: x0..x7 are arguments, x8 is the return argument pointer.
    addBasicRegister(BasicRegister::reg3264(8, "w8", "x8", nosave_flags));
    addBasicRegister(BasicRegister::reg3264(7, "w7", "x7", nosave_flags));
    addBasicRegister(BasicRegister::reg3264(6, "w6", "x6", nosave_flags));
    addBasicRegister(BasicRegister::reg3264(5, "w5", "x5", nosave_flags));
    addBasicRegister(BasicRegister::reg3264(4, "w4", "x4", nosave_flags));
    addBasicRegister(BasicRegister::reg3264(3, "w3", "x3", nosave_flags));
    addBasicRegister(BasicRegister::reg3264(2, "w2", "x2", nosave_flags));
    addBasicRegister(BasicRegister::reg3264(1, "w1", "x1", nosave_flags));
    addBasicRegister(BasicRegister::reg3264(0, "w0", "x0", nosave_flags));
    // x16..x18 are reserved for special purposes.  We already have
    // tons of registers so we mark the special ones as non-allocatable.
    addBasicRegister
        (BasicRegister::reg3264(16, "w16", "x16", save_flags | noalloc));
    addBasicRegister
        (BasicRegister::reg3264(17, "w17", "x17", save_flags | noalloc));
    addBasicRegister
        (BasicRegister::reg3264(18, "w18", "x18", save_flags | noalloc));
    // Callee-saved registers.
    addBasicRegister(BasicRegister::reg3264(19, "w19", "x19", save_flags));
    addBasicRegister(BasicRegister::reg3264(20, "w20", "x20", save_flags));
    addBasicRegister(BasicRegister::reg3264(21, "w21", "x21", save_flags));
    addBasicRegister(BasicRegister::reg3264(22, "w22", "x22", save_flags));
    addBasicRegister(BasicRegister::reg3264(23, "w23", "x23", save_flags));
    addBasicRegister(BasicRegister::reg3264(24, "w24", "x24", save_flags));
    addBasicRegister(BasicRegister::reg3264(25, "w25", "x25", save_flags));
    addBasicRegister(BasicRegister::reg3264(26, "w26", "x26", save_flags));
    addBasicRegister(BasicRegister::reg3264(27, "w27", "x27", save_flags));
    addBasicRegister(BasicRegister::reg3264(28, "w28", "x28", save_flags));
    addBasicRegister(BasicRegister::reg64(29, "fp", save_flags));
    addBasicRegister
        (BasicRegister::reg64
            (30, "lr", save_flags | BasicRegister::Link));
    BasicRegister sp =
        BasicRegister::reg64
            (31, "sp", addr_only_flags | BasicRegister::StackPointer
                                       | BasicRegister::NoAllocate);
    addBasicRegister(sp);
    setStackPointer(sp);
    addBasicRegister
        (BasicRegister::reg64
            (32, "pc", addr_only_flags | BasicRegister::ProgramCounter
                                       | BasicRegister::NoAllocate));

    // Specify the argument registers: x0..x7
    addArgumentRegister(0);
    addArgumentRegister(1);
    addArgumentRegister(2);
    addArgumentRegister(3);
    addArgumentRegister(4);
    addArgumentRegister(5);
    addArgumentRegister(6);
    addArgumentRegister(7);
}

Platform_ARMv8a::~Platform_ARMv8a()
{
}

BasicRegister::Size Platform_ARMv8a::nativeWordSize() const
{
    return BasicRegister::Size64;
}

// Determine if a constant can be used as immediate in an ARMv8a move.
static bool isOperandMoveConstantARMv8a
    (Insn::ImmValue value, BasicRegister::Size size)
{
    // Move immediates can be any 16-bit value that can be shifted
    // into place by 0, 16, 32, or 48 bits.
    if (size == BasicRegister::Size64) {
        if ((value & 0x000000000000FFFFULL) == value)
            return true;
        if ((value & 0x00000000FFFF0000ULL) == value)
            return true;
        if ((value & 0x0000FFFF00000000ULL) == value)
            return true;
        if ((value & 0xFFFF000000000000ULL) == value)
            return true;
        return false;
    } else {
        uint32_t val = (uint32_t)value;
        if ((val & 0x0000FFFFU) == val)
            return true;
        if ((val & 0xFFFF0000U) == val)
            return true;
        return false;
    }
}

// Determine if a constant can be used as immediate in an AND/OR/XOR insn.
// Logical immediates consist of a repeating bit pattern and a rotation count.
//
// https://stackoverflow.com/questions/30904718/range-of-immediate-values-in-armv8-a64-assembly/33265035#33265035
//
// Summary of the simplest explanation from that stackoverflow page:
//
// 1. Make a pattern of X > 0 consecutive zero bits, followed by Y > 0
//    consecutive one bits, where X + Y is a power of two.
// 2. Repeat the pattern until the entire 32-bit or 64-bit word is filled.
// 3. Apply a rotation to the pattern of between 0 and (X + Y - 1) bits.
static bool isOperandLogicalConstantARMv8a
    (Insn::ImmValue value, BasicRegister::Size size)
{
    // If the size is 32-bit, then convert it into a 64-bit value by
    // duplicating the word into the low and high halves.  Then we only
    // have to worry about the 64-bit case.
    if (size == BasicRegister::Size32)
        value = (value << 32) | (uint32_t)value;

    // All-zeroes and all-ones cannot be represented.
    if (value == 0 || value == ~0ULL)
        return false;

    // Rotate the value right until we have a 1 in the least significant bit.
    while ((value & 1) == 0)
        value = (value >> 1) | (value << 63);

    // Rotate the value left until the highest bit is zero to align the runs.
    while ((value & 0x8000000000000000ULL) != 0)
        value = (value << 1) | (value >> 63);

    // Count how many 1's and 0's we have in the bottom-most run.
    unsigned ones = 1;
    while ((value & (1ULL << ones)) != 0)
        ++ones;
    unsigned zeroes = ones;
    while (zeroes < 64 && (value & (1ULL << zeroes)) == 0)
        ++zeroes;
    zeroes -= ones;

    // If the run is 64 bits in length, then we are done.
    unsigned runSize = ones + zeroes;
    if (runSize == 64)
        return true;

    // If the run is not a power of two, then this is not a valid logical mask.
    if (runSize != 2 && runSize != 4 && runSize != 8 &&
            runSize != 16 && runSize != 32)
        return false;

    // Check that the run is repeated throughout the rest of the word.
    Insn::ImmValue run = value & ((1ULL << runSize) - 1);
    for (unsigned offset = runSize; offset < 64; offset += runSize) {
        Insn::ImmValue nextRun = (value >> offset) & ((1ULL << runSize) - 1);
        if (nextRun != run)
            return false;
    }
    return true;
}

bool Platform_ARMv8a::validateImmediate
    (Insn::Type type, Insn::ImmValue value, BasicRegister::Size size) const
{
    switch (type) {
    case Insn::ADDI:
    case Insn::CMPI:
    case Insn::CMPNI:
    case Insn::SUBI:
        // Can be a 12-bit constant shifted by 0 or 12 bit positions.
        if ((value & 0x00000FFFU) == value)
            return true;
        if ((value & 0x00FFF000U) == value)
            return true;
        break;

    case Insn::ANDI:
    case Insn::ORI:
    case Insn::XORI:
        return isOperandLogicalConstantARMv8a(value, size);

    case Insn::MOVI:
        // We can use "MOV (wide immediate)" or "MOV (bitmask immediate)".
        return isOperandMoveConstantARMv8a(value, size) ||
               isOperandLogicalConstantARMv8a(value, size);

    case Insn::MOVN:
        return isOperandMoveConstantARMv8a(value, size);

    case Insn::ASRI:
    case Insn::LSLI:
    case Insn::LSRI:
    case Insn::ROLI:
    case Insn::RORI:
        if (size == BasicRegister::Size64)
            return value < 64;
        else
            return value < 32;

    case Insn::LD8:
    case Insn::LD8S:
    case Insn::ST8:
        return value <= 4095;

    case Insn::LD16:
    case Insn::LD16S:
    case Insn::ST16:
        return (value & 1) == 0 && value <= 8190;

    case Insn::LD32:
    case Insn::LD32S:
    case Insn::ST32:
        return (value & 3) == 0 && value <= 16380;

    case Insn::LD64:
    case Insn::ST64:
        return (value & 7) == 0 && value <= 32760;

    default: break;
    }
    return false;
}

void Platform_ARMv8a::moveImm
    (CodeGenerator *code, const SizedRegister &reg, Insn::ImmValue value) const
{
    Insn insn;
    if (reg.size() == BasicRegister::Size64) {
        if ((value & 0x000000000000FFFFULL) == value) {
            // MOVZ reg, imm
            code->addInsn(Insn::moveImm(Insn::MOVW, reg, value));
        } else if ((value & 0x00000000FFFF0000ULL) == value) {
            // MOVZ reg, imm, LSL #16
            insn = Insn::moveImm(Insn::MOVW, reg, value >> 16);
            insn.setModifier(Insn::MOD_LSL);
            insn.setShift(16);
            code->addInsn(insn);
        } else if ((value & 0x0000FFFF00000000ULL) == value) {
            // MOVZ reg, imm, LSL #32
            insn = Insn::moveImm(Insn::MOVW, reg, value >> 32);
            insn.setModifier(Insn::MOD_LSL);
            insn.setShift(32);
            code->addInsn(insn);
        } else if ((value & 0xFFFF000000000000ULL) == value) {
            // MOVZ reg, imm, LSL #48
            insn = Insn::moveImm(Insn::MOVW, reg, value >> 48);
            insn.setModifier(Insn::MOD_LSL);
            insn.setShift(48);
            code->addInsn(insn);
        } else if (value == ~((~value) & 0x000000000000FFFFULL)) {
            // MOVN reg, imm
            code->addInsn(Insn::moveImm(Insn::MOVN, reg, (~value) & 0xFFFFU));
        } else if (value == ~((~value) & 0x00000000FFFF0000ULL)) {
            // MOVN reg, imm, LSL #16
            insn = Insn::moveImm(Insn::MOVN, reg, ((~value) >> 16) & 0xFFFFU);
            insn.setModifier(Insn::MOD_LSL);
            insn.setShift(16);
            code->addInsn(insn);
        } else if (value == ~((~value) & 0x0000FFFF00000000ULL)) {
            // MOVN reg, imm, LSL #32
            insn = Insn::moveImm(Insn::MOVN, reg, ((~value) >> 32) & 0xFFFFU);
            insn.setModifier(Insn::MOD_LSL);
            insn.setShift(32);
            code->addInsn(insn);
        } else if (value == ~((~value) & 0xFFFF000000000000ULL)) {
            // MOVN reg, imm, LSL #48
            insn = Insn::moveImm(Insn::MOVN, reg, ((~value) >> 48) & 0xFFFFU);
            insn.setModifier(Insn::MOD_LSL);
            insn.setShift(48);
            code->addInsn(insn);
        } else if (isOperandLogicalConstantARMv8a(value, reg.size())) {
            // MOV reg, imm (bitmask immediate)
            code->addInsn(Insn::moveImm(Insn::MOVI, reg, value));
        } else {
            // LDR reg, =imm
            code->addInsn(Insn::moveImm(Insn::LDI, reg, value));
        }
    } else {
        uint32_t val = (uint32_t)value;
        if ((val & 0x0000FFFFU) == val) {
            // MOVZ reg, imm
            code->addInsn(Insn::moveImm(Insn::MOVI, reg, val));
        } else if ((val & 0xFFFF0000U) == val) {
            // MOVZ reg, imm, LSL #16
            insn = Insn::moveImm(Insn::MOVI, reg, val >> 16);
            insn.setModifier(Insn::MOD_LSL);
            insn.setShift(16);
            code->addInsn(insn);
        } else if (val == ~(((~val) & 0x0000FFFFU))) {
            // MOVN reg, imm
            code->addInsn(Insn::moveImm(Insn::MOVN, reg, (~val) & 0xFFFFU));
        } else if (val == ~(((~val) & 0xFFFF0000U))) {
            // MOVN reg, imm, LSL #16
            insn = Insn::moveImm(Insn::MOVN, reg, ((~val) >> 16) & 0xFFFFU);
            insn.setModifier(Insn::MOD_LSL);
            insn.setShift(16);
            code->addInsn(insn);
        } else if (isOperandLogicalConstantARMv8a(val, reg.size())) {
            // MOV reg, imm (bitmask immediate)
            code->addInsn(Insn::moveImm(Insn::MOVI, reg, val));
        } else {
            // MOVZ reg, LOW(imm)
            code->addInsn(Insn::moveImm(Insn::MOVI, reg, val & 0xFFFFU));
            // MOVK reg, HIGH(imm), LSL #16
            insn = Insn::binaryImm(Insn::MOVT, reg, reg, (val >> 16) & 0xFFFFU);
            insn.setModifier(Insn::MOD_LSL);
            insn.setShift(16);
            code->addInsn(insn);
        }
    }
}

} // namespace gencrypto
