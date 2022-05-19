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

#include "registry.h"

namespace gencrypto
{

std::vector<Registration> Registration::registrations;

Registration::Registration(const Registration &other)
    : m_name(other.m_name)
    , m_variant(other.m_variant)
    , m_platform(other.m_platform)
    , m_generate(other.m_generate)
    , m_test(other.m_test)
    , m_generateAVR(other.m_generateAVR)
    , m_testAVR(other.m_testAVR)
{
}

Registration &Registration::operator=(const Registration &other)
{
    if (this != &other) {
        m_name = other.m_name;
        m_variant = other.m_variant;
        m_platform = other.m_platform;
        m_generate = other.m_generate;
        m_test = other.m_test;
        m_generateAVR = other.m_generateAVR;
        m_testAVR = other.m_testAVR;
    }
    return *this;
}

std::string Registration::qualifiedName() const
{
    std::string qual(m_name);
    if (!m_variant.empty()) {
        qual.push_back(':');
        qual += m_variant;
    }
    if (!m_platform.empty()) {
        qual.push_back(':');
        qual += m_platform;
    }
    return qual;
}

void Registration::registerFunction
    (const char *name, const char *variant, const char *platform,
     GenerateHandler_t gen, TestHandler_t test)
{
    Registration reg;
    if (name)
        reg.setName(name);
    if (variant)
        reg.setVariant(variant);
    if (platform)
        reg.setPlatform(platform);
    reg.setGenerate(gen);
    reg.setTest(test);
    Registration::registrations.push_back(reg);
}

void Registration::registerFunctionAVR
    (const char *name, const char *variant, const char *platform,
     AVRGenerateHandler_t gen, AVRTestHandler_t test)
{
    Registration reg;
    if (name)
        reg.setName(name);
    if (variant)
        reg.setVariant(variant);
    if (platform)
        reg.setPlatform(platform);
    reg.setGenerateAVR(gen);
    reg.setTestAVR(test);
    Registration::registrations.push_back(reg);
}

bool Registration::operator<(const Registration &other) const
{
    if (m_name < other.m_name)
        return true;
    else if (m_name != other.m_name)
        return false;
    if (m_variant < other.m_variant)
        return true;
    else if (m_variant != other.m_variant)
        return false;
    return m_platform < other.m_platform;
}

Registration Registration::find(const std::string &qualifiedName)
{
    size_t index;
    for (index = 0; index < registrations.size(); ++index) {
        if (registrations[index].qualifiedName() == qualifiedName)
            return registrations[index];
    }
    return Registration();
}

} // namespace gencrypto
