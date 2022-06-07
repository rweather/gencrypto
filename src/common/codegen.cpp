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

#include "codegen.h"

namespace gencrypto
{

CodeGenerator::CodeGenerator(Platform *platform)
    : m_platform(platform)
    , m_allocationSize(platform->nativeWordSize())
    , m_allocatedRegs(0)
    , m_usedRegs(0)
    , m_nextArgumentReg(0)
    , m_nextArgumentOffset(0)
    , m_locals(0)
{
}

CodeGenerator::~CodeGenerator()
{
}

Reg CodeGenerator::sp() const
{
    return Reg(m_platform->stackPointer());
}

void CodeGenerator::setAllocationSize(BasicRegister::Size size)
{
    std::vector<BasicRegister>::const_iterator it;
    for (it = m_platform->m_registers.cbegin();
            it != m_platform->m_registers.cend(); ++it) {
        if (it->hasFlag(BasicRegister::Data) && it->hasSize(size)) {
            m_allocationSize = size;
            break;
        }
    }
}

Reg CodeGenerator::allocateReg
    (size_t size, uint16_t flags1, uint16_t flags2,
     uint16_t flags3, uint16_t flags4)
{
    if (!size)
        throw std::invalid_argument("cannot allocate zero-sized registers");
    Reg reg = allocate(size, flags1);
    if (!reg.isNull())
        return reg;
    reg = allocate(size, flags2);
    if (!reg.isNull())
        return reg;
    reg = allocate(size, flags3);
    if (!reg.isNull())
        return reg;
    reg = allocate(size, flags4);
    if (reg.isNull()) {
        std::string msg = "cannot allocate a register with ";
        msg += std::to_string(size);
        msg += " bits";
        throw std::invalid_argument(msg);
    }
    return reg;
}

void CodeGenerator::releaseReg(Reg &reg)
{
    for (size_t index = 0; index < reg.numRegs(); ++index) {
        uint8_t number = reg.reg(index).number();
        m_allocatedRegs &= ~((((RegMask)1)) << number);
    }
    reg = Reg();
}

Reg CodeGenerator::addArgument(ArgType type)
{
    size_t nsize = (size_t)(m_platform->nativeWordSize());
    size_t asize = (size_t)(m_platform->addressWordSize());
    Reg reg;

    // Determine how many bits are in the argument.
    size_t size = nsize;
    switch (type) {
    case ARG_INT8:
    case ARG_UINT8:     size = 8; break;
    case ARG_INT16:
    case ARG_UINT16:    size = 16; break;
    case ARG_INT32:
    case ARG_UINT32:    size = 32; break;
    case ARG_INT64:
    case ARG_UINT64:    size = 64; break;
    case ARG_PTR:       size = asize; break;
    }

    // Round smaller values up to the native word size.
    if (size < nsize)
        size = nsize;

    // How many registers do we need to hold the value?
    size_t count;
    if (type == ARG_PTR) {
        // Pointers are assumed to always need one register.
        count = 1;
        nsize = asize;
    } else if (size == 64 && nsize < asize && asize >= 64) {
        // Special case when nsize is less than 64 but the underlying
        // platform is actually 64-bit.  This may happen if we are
        // emulating a 32-bit platform on top of a 64-bit one.
        count = 1;
        nsize = asize;
    } else {
        count = size / nsize;
    }

    // Allocate as many actual registers as we can.
    while (count > 0 && m_nextArgumentReg < m_platform->m_arguments.size()) {
        BasicRegister basic = m_platform->m_arguments[m_nextArgumentReg++];
        reg.addRegister(basic, (BasicRegister::Size)nsize);
        m_allocatedRegs |= ((((RegMask)1)) << basic.number());
        m_usedRegs |= ((((RegMask)1)) << basic.number());
        --count;
    }

    // The remaining words are loaded from the stack instead.
    size_t totalSize = count * nsize;
    size_t offset = m_nextArgumentOffset;
    while (count > 0) {
        Reg temp;
        if (type == ARG_PTR)
            temp = allocate(nsize, BasicRegister::Address);
        else
            temp = allocate(nsize, BasicRegister::Data);
        if (temp.isNull())
            throw std::invalid_argument("cannot allocate argument register");
        // TODO: ldarg(temp, offset);
        reg.addRegister(temp.reg(0));
        m_allocatedRegs |= ((((RegMask)1)) << temp.number(0));
        m_usedRegs |= ((((RegMask)1)) << temp.number(0));
        offset += nsize;
        --count;
    }
    totalSize = (totalSize + asize - 1) & ~(asize - 1);
    m_nextArgumentOffset += totalSize;

    // Reverse the register order if the platform is big-endian.
    if (m_platform->hasFeature(Platform::BigEndian))
        reg = reg.reversed();

    // Return the argument register to the caller.
    return reg;
}

void CodeGenerator::setupLocals(size_t sizeLocals)
{
    // Round the local variable size up to a multiple of the address size.
    // May be increased even further when code generation occurs.
    size_t asize = (size_t)(m_platform->addressWordSize());
    m_locals = (sizeLocals + asize - 1) & ~(asize - 1);
}

void CodeGenerator::setupPermutation(Reg &state, size_t sizeLocals)
{
    setupLocals(sizeLocals);
    state = addArgument(ARG_PTR);
}

void CodeGenerator::setupPermutationWithCount
    (Reg &state, Reg &count, size_t sizeLocals, ArgType type)
{
    setupLocals(sizeLocals);
    state = addArgument(ARG_PTR);
    count = addArgument(type);
}

void CodeGenerator::reschedule(int8_t offset, size_t index)
{
    if (index >= m_insns.size())
        return;
    m_insns[m_insns.size() - index - 1].reschedule(offset);
}

/**
 * \brief Allocates a register of a specific bit size.
 *
 * \param size Size of the desired register in bits.
 * \param flags1 Flags that indicate the type of register that
 * we are looking for.  All flags must be present.
 *
 * \return The allocated register.
 */
Reg CodeGenerator::allocate(size_t size, uint16_t flags)
{
    // Bail out if we are looking for nothing.
    if (!flags)
        return Reg();

    // Determine how many basic registers we need and of what size.
    BasicRegister::Size rsize = m_allocationSize;
    if ((flags & BasicRegister::Address) != 0) {
        // Address registers must always be the platform's address word size.
        rsize = m_platform->addressWordSize();
    }
    size_t limbSize = (size_t)rsize;
    size_t count = (size + limbSize - 1) / limbSize;

    // Build the register to return out of one or more basic registers.
    std::vector<BasicRegister>::const_iterator it;
    Reg reg;
    for (it = m_platform->m_registers.cbegin();
            it != m_platform->m_registers.cend(); ++it) {
        if (reg.numRegs() == count)
            break;      // We have enough basic registers now.
        if ((m_allocatedRegs & (((RegMask)1) << it->number())) != 0)
            continue;   // Already allocated.
        if (!it->hasSize(rsize))
            continue;   // Does not support the required size.
        if ((it->flags() & flags) != flags)
            continue;   // Does not support the features we want.
        if (it->hasFlag(BasicRegister::NoAllocate))
            continue;   // Explicitly disabled for register allocation.
        reg.addRegister(*it, rsize);
    }
    if (reg.numRegs() != count) {
        // We could not find enough registers to fulfill the request.
        return Reg();
    }

    // Mark the registers as allocated and in-use.
    for (size_t index = 0; index < reg.numRegs(); ++index) {
        uint8_t number = reg.reg(index).number();
        m_allocatedRegs |= ((((RegMask)1)) << number);
        m_usedRegs |= ((((RegMask)1)) << number);
    }

    // Set the actual register size and return the register.
    reg.setSize(size);
    if (reg.fullSize() != size)
        reg.setZeroFill(false);
    return reg;
}

} // namespace gencrypto
