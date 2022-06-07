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

#include "platform.h"
#include <stdexcept>

namespace gencrypto
{

BasicRegister::Private::Private()
    : refCount(1)
    , number(255)
    , sizes(Size8)
    , flags(0)
{
}

BasicRegister::Private::Private(const Private &other)
    : refCount(1)
    , number(other.number)
    , sizes(other.sizes)
    , flags(other.flags)
    , name8(other.name8)
    , name16(other.name16)
    , name32(other.name32)
    , name64(other.name64)
    , addrName(other.addrName)
{
}

BasicRegister::BasicRegister(const BasicRegister &other)
    : p(other.p)
{
    if (p)
        ++(p->refCount);
}

BasicRegister::~BasicRegister()
{
    if (p) {
        --(p->refCount);
        if (p->refCount == 0)
            delete p;
    }
}

BasicRegister &BasicRegister::operator=(const BasicRegister &other)
{
    if (p != other.p) {
        if (p) {
            --(p->refCount);
            if (p->refCount == 0)
                delete p;
        }
        if (other.p) {
            ++(other.p->refCount);
        }
        p = other.p;
    }
    return *this;
}

BasicRegister::Size BasicRegister::maxSize() const
{
    if (hasSize(Size64))
        return Size64;
    if (hasSize(Size32))
        return Size32;
    if (hasSize(Size16))
        return Size16;
    return Size8;
}

std::string BasicRegister::addressName() const
{
    if (!p)
        return std::string();
    if (!p->addrName.empty())
        return p->addrName;
    if (hasSize(Size64))
        return p->name64;
    if (hasSize(Size32))
        return p->name32;
    return p->name16;
}

std::string BasicRegister::nameForSize(Size size) const
{
    if (size == BasicRegister::Size8)
        return name8();
    else if (size == BasicRegister::Size16)
        return name16();
    else if (size == BasicRegister::Size32)
        return name32();
    else
        return name64();
}

BasicRegister BasicRegister::reg32
    (uint8_t number, const std::string &name, uint16_t flags)
{
    BasicRegister reg;
    reg.setNumber(number);
    reg.setSizes(Size32);
    reg.setName32(name);
    reg.setFlags(flags);
    return reg;
}

BasicRegister BasicRegister::reg64
    (uint8_t number, const std::string &name, uint16_t flags)
{
    BasicRegister reg;
    reg.setNumber(number);
    reg.setSizes(Size64);
    reg.setName64(name);
    reg.setFlags(flags);
    return reg;
}

BasicRegister BasicRegister::reg3264
    (uint8_t number, const std::string &name32,
     const std::string &name64, uint16_t flags)
{
    BasicRegister reg;
    reg.setNumber(number);
    reg.setSizes(Size32 | Size64);
    reg.setName32(name32);
    reg.setName64(name64);
    reg.setFlags(flags);
    return reg;
}

const BasicRegister::Private *BasicRegister::read() const
{
    if (p)
        return p;
    p = new Private();
    return p;
}

BasicRegister::Private *BasicRegister::write()
{
    if (p) {
        if (p->refCount > 1) {
            Private *newp = new Private(*p);
            --(p->refCount);
            p = newp;
        }
    } else {
        p = new Private();
    }
    return p;
}

SizedRegister::SizedRegister
        (const BasicRegister &reg, BasicRegister::Size size)
    : m_reg(reg)
    , m_size(size)
{
    if ((size & reg.sizes()) != size) {
        std::string msg = "register ";
        msg += reg.addressName();
        msg += " does not support the ";
        msg += std::to_string((int)size);
        msg += "-bit size";
        throw std::invalid_argument(msg);
    }
}

Reg::Reg()
    : m_size(0)
    , m_fullSize(0)
    , m_zeroFill(true)
{
}

Reg::Reg(const Reg &other)
    : m_size(other.m_size)
    , m_fullSize(other.m_fullSize)
    , m_zeroFill(other.m_zeroFill)
    , m_regs(other.m_regs)
{
}

Reg::Reg(const SizedRegister &reg)
    : m_size(0)
    , m_fullSize(0)
    , m_zeroFill(true)
{
    addRegister(reg);
}

Reg::Reg(const BasicRegister &reg)
    : m_size(0)
    , m_fullSize(0)
    , m_zeroFill(true)
{
    addRegister(reg);
}

Reg::Reg(const BasicRegister &reg, BasicRegister::Size size)
    : m_size(0)
    , m_fullSize(0)
    , m_zeroFill(true)
{
    addRegister(reg, size);
}

Reg &Reg::operator=(const Reg &other)
{
    if (&other != this) {
        m_size = other.m_size;
        m_fullSize = other.m_fullSize;
        m_zeroFill = other.m_zeroFill;
        m_regs = other.m_regs;
    }
    return *this;
}

void Reg::setSize(size_t size)
{
    if (size > m_fullSize || size <= (m_fullSize - limbSize())) {
        throw std::invalid_argument("invalid size for Reg object");
    }
    m_size = size;
}

size_t Reg::limbSize() const
{
    if (m_regs.size() > 0)
        return (size_t)(m_regs[0].size());
    else
        return 0;
}

void Reg::addRegister(const SizedRegister &reg)
{
    size_t index;

    // Check that the basic register isn't already present.
    for (index = 0; index < m_regs.size(); ++index) {
        if (reg.reg().number() == m_regs[index].reg().number()) {
            std::string msg;
            msg += reg.name();
            msg += " appears twiwce in a Reg instance";
            throw std::invalid_argument(msg);
        }
    }

    // All sized registers must have the same size.
    if (m_regs.size() > 0 && m_regs[0].size() != reg.size()) {
        std::string msg;
        msg += reg.name();
        msg += " is not the same size as other Reg members such as ";
        msg += m_regs[0].name();
        throw std::invalid_argument(msg);
    }

    // Add the sized register to the full register.
    m_regs.push_back(reg);

    // Update the size of the full register.
    m_size += (size_t)(reg.size());
    m_fullSize += (size_t)(reg.size());
}

Reg Reg::reversed() const
{
    if (m_size != m_fullSize)
        throw std::invalid_argument("cannot reverse an odd-sized register");
    Reg result;
    result.m_size = m_size;
    result.m_fullSize = m_fullSize;
    result.m_zeroFill = true;
    for (size_t index = m_regs.size(); index > 0; --index)
        result.m_regs.push_back(m_regs[index - 1]);
    return result;
}

Reg Reg::subset(size_t start, size_t len) const
{
    // Validate the range of the subset.
    if (len == 0)
        len = m_size;
    if (m_regs.empty() || start >= m_size)
        return Reg();
    if ((start + len) > m_size)
        len = m_size - start;
    if (len == 0)
        return Reg();
    size_t rsize = (size_t)(m_regs[0].size());
    if ((start % rsize) != 0) {
        std::string msg = "start of subset is not a multiple of ";
        msg += std::to_string(rsize);
        throw std::invalid_argument(msg);
    }

    // Create the subset.
    Reg result;
    size_t from, to;
    if ((start + len) < m_size) {
        // Must have a register-aligned subset.
        if ((len % rsize) != 0) {
            std::string msg = "length of subset is not a multiple of ";
            msg += std::to_string(rsize);
            throw std::invalid_argument(msg);
        }
        result.m_size = len / rsize;
        result.m_fullSize = result.m_size;
        result.m_zeroFill = true;
        from = start / rsize;
        to = (start + len) / rsize;
    } else {
        // Everything from start to the end of this register.
        result.m_size = m_size - start;
        result.m_fullSize = m_fullSize - start;
        result.m_zeroFill = m_zeroFill;
        from = start / rsize;
        to = m_regs.size();
    }
    while (from <= to) {
        result.m_regs.push_back(m_regs[from]);
        ++from;
    }
    return result;
}

} // namespace gencrypto
