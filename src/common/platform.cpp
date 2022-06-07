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

namespace gencrypto
{

Platform::Platform()
    : m_features(0)
{
}

Platform::~Platform()
{
}

BasicRegister::Size Platform::addressWordSize() const
{
    return nativeWordSize();
}

SizedRegister Platform::registerForName(const std::string &name) const
{
    std::vector<BasicRegister>::const_iterator it;
    if (name.empty()) {
        // Avoid accidental matching of a basic register that does
        // not have a name for one or more of the basic sizes.
        return SizedRegister();
    }
    for (it = m_registers.cbegin(); it != m_registers.cend(); ++it) {
        if (name == it->name64())
            return SizedRegister(*it, BasicRegister::Size64);
        else if (name == it->name32())
            return SizedRegister(*it, BasicRegister::Size32);
        else if (name == it->name16())
            return SizedRegister(*it, BasicRegister::Size16);
        else if (name == it->name8())
            return SizedRegister(*it, BasicRegister::Size8);
    }
    return SizedRegister();
}

BasicRegister Platform::registerForNumber(uint8_t number) const
{
    std::vector<BasicRegister>::const_iterator it;
    for (it = m_registers.cbegin(); it != m_registers.cend(); ++it) {
        if (it->number() == number)
            return *it;
    }
    return BasicRegister();
}

void Platform::beginWrite() const
{
}

} // namespace gencrypto
