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

#include "insns.h"

namespace gencrypto
{

Insn::Insn(const Insn &other)
    : p(other.p)
{
    if (p)
        ++(p->refCount);
}

Insn::~Insn()
{
    if (p && --(p->refCount) == 0)
        delete p;
}

Insn &Insn::operator=(const Insn &other)
{
    if (p != other.p) {
        if (p && --(p->refCount) == 0)
            delete p;
        p = other.p;
        if (p)
            ++(p->refCount);
    }
    return *this;
}

Insn Insn::bare(Type type)
{
    Insn insn;
    insn.setType(type);
    return insn;
}

Insn Insn::unary
    (Type type, const SizedRegister &dest, const SizedRegister &src,
     Option option)
{
    Insn insn;
    insn.setType(type);
    insn.setDest(dest);
    insn.setSrc1(src);
    insn.setOption(option);
    return insn;
}

Insn Insn::binary
    (Type type, const SizedRegister &dest, const SizedRegister &src1,
     const SizedRegister &src2, Option option)
{
    Insn insn;
    insn.setType(type);
    insn.setDest(dest);
    insn.setSrc1(src1);
    insn.setSrc2(src2);
    insn.setOption(option);
    return insn;
}

Insn Insn::binary
    (Type type, const SizedRegister &dest, const SizedRegister &src1,
     const SizedRegister &src2, Modifier modifier, uint8_t shift,
     Option option)
{
    Insn insn;
    insn.setType(type);
    insn.setDest(dest);
    insn.setSrc1(src1);
    insn.setSrc2(src2);
    if (modifier != MOD_NONE && shift != 0) {
        insn.setModifier(modifier);
        insn.setShift(shift);
    }
    insn.setOption(option);
    return insn;
}

Insn Insn::binaryImm
    (Type type, const SizedRegister &dest, const SizedRegister &src1,
     ImmValue immValue, Option option)
{
    Insn insn;
    insn.setType(type);
    insn.setDest(dest);
    insn.setSrc1(src1);
    insn.setImmValue(immValue);
    insn.setOption(option);
    return insn;
}

Insn Insn::moveImm
    (Type type, const SizedRegister &dest, ImmValue immValue, Option option)
{
    Insn insn;
    insn.setType(type);
    insn.setDest(dest);
    insn.setImmValue(immValue);
    insn.setOption(option);
    return insn;
}

Insn Insn::branch(Type type, Label label)
{
    Insn insn;
    insn.setType(type);
    insn.setLabel(label);
    return insn;
}

Insn::Private::Private()
    : refCount(1)
    , type(Insn::Unknown)
    , modifier(Insn::MOD_NONE)
    , shift(0)
    , fields(0)
    , reschedule(0)
    , option(Insn::NONE)
    , immValue(0)
{
}

Insn::Private::Private(const Private &other)
    : refCount(1)
    , type(other.type)
    , modifier(other.modifier)
    , shift(other.shift)
    , fields(other.fields)
    , reschedule(other.reschedule)
    , option(other.option)
    , dest(other.dest)
    , src1(other.src1)
    , src2(other.src2)
    , immValue(other.immValue)
{
}

const Insn::Private *Insn::read() const
{
    if (!p)
        p = new Private();
    return p;
}

Insn::Private *Insn::write()
{
    return 0;
}

} // namespace gencrypto
