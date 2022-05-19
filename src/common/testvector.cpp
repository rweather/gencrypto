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

#include "testvector.h"
#include <algorithm>
#include <iostream>
#include <cstring>

namespace gencrypto
{

std::string TestVector::valueAsString(const std::string &key) const
{
    std::map<std::string, std::string>::const_iterator it;
    it = m_map.find(key);
    if (it != m_map.end())
        return it->second;
    else
        return std::string();
}

std::vector<unsigned char> TestVector::valueAsBinary(const std::string &key) const
{
    std::vector<unsigned char> result;
    std::string value = valueAsString(key);
    int val = 0;
    bool nibble = false;
    for (size_t index = 0; index < value.size(); ++index) {
        char ch = value[index];
        if (ch >= '0' && ch <= '9')
            val = val * 16 + (ch - '0');
        else if (ch >= 'A' && ch <= 'F')
            val = val * 16 + (ch - 'A' + 10);
        else if (ch >= 'a' && ch <= 'f')
            val = val * 16 + (ch - 'a' + 10);
        else
            continue;
        nibble = !nibble;
        if (!nibble) {
            result.push_back((unsigned char)val);
            val = 0;
        }
    }
    return result;
}

int TestVector::valueAsInt(const std::string &key, int defaultValue) const
{
    std::string value = valueAsString(key);
    if (value.empty())
        return defaultValue;
    else
        return std::stoi(value);
}

bool TestVector::populate
    (unsigned char *buf, size_t size, const char *name) const
{
    std::vector<unsigned char> data = valueAsBinary(name);
    if (data.size() != size) {
        memset(buf, 0, size);
        return false;
    }
    for (size_t index = 0; index < size; ++index) {
        buf[index] = data[index];
    }
    return true;
}

static void reportHex(unsigned char value)
{
    static char const hexchars[] = "0123456789ABCDEF";
    std::cout << hexchars[(value >> 4) & 0x0F];
    std::cout << hexchars[value & 0x0F];
}

static void reportMismatch
    (const unsigned char *buf, size_t size,
     const std::vector<unsigned char> &data)
{
    size_t index;
    std::cout << std::endl;
    std::cout << "    actual   = ";
    for (index = 0; index < size; ++index)
        reportHex(buf[index]);
    std::cout << std::endl;
    std::cout << "    expected = ";
    for (index = 0; index < data.size(); ++index)
        reportHex(data[index]);
    std::cout << std::endl;
}

bool TestVector::check
    (const unsigned char *buf, size_t size, const char *name) const
{
    std::vector<unsigned char> data = valueAsBinary(name);
    if (data.size() != size) {
        reportMismatch(buf, size, data);
        return false;
    }
    for (size_t index = 0; index < size; ++index) {
        if (buf[index] != data[index]) {
            reportMismatch(buf, size, data);
            return false;
        }
    }
    return true;
}

TestVectorFile::TestVectorFile()
    : m_groups(0)
{
}

TestVectorFile::~TestVectorFile()
{
    clear();
}

// Trim whitespace from the start and end of a string.
static std::string trim(const std::string &str)
{
    std::string temp(str);
    temp.erase(temp.find_last_not_of(" \t\r\n") + 1);
    temp.erase(0, temp.find_first_not_of(" \t\r\n"));
    return temp;
}

void TestVectorFile::load(std::istream &file)
{
    std::string line;
    bool inGroup = false;
    TestVectorGroup *group;
    clear();
    addGroup();
    while (std::getline(file, line)) {
        size_t pos = line.find('=');
        if (pos == std::string::npos)
            continue;
        std::string name = trim(line.substr(0, pos));
        std::string value = trim(line.substr(pos + 1));
        if (name.rfind("Function", 0) != std::string::npos) {
            // Add a function name to the current group or start a new group.
            if (inGroup) {
                addGroup();
                inGroup = false;
            }
            m_groups->names.push_back(value);
        } else if (name.rfind("Name", 0) != std::string::npos) {
            // Start of a new test vector.
            group = m_groups;
            group->vectors.push_back(TestVector());
            group->vectors[group->vectors.size() - 1].insert(name, value);
            inGroup = true;
        } else {
            // Add a field to the current test vector.
            group = m_groups;
            if (group->vectors.empty())
                group->vectors.push_back(TestVector());
            group->vectors[group->vectors.size() - 1].insert(name, value);
            inGroup = true;
        }
    }
}

TestVectorList TestVectorFile::testsFor(const std::string &name) const
{
    const TestVectorGroup *group = m_groups;
    while (group != 0) {
        if (std::find(group->names.begin(), group->names.end(), name)
                != group->names.end())
            return group->vectors;
        group = group->next;
    }
    return TestVectorList();
}

void TestVectorFile::clear()
{
    TestVectorGroup *group = m_groups;
    TestVectorGroup *next;
    while (group != 0) {
        next = group->next;
        delete group;
        group = next;
    }
    m_groups = 0;
}

void TestVectorFile::addGroup()
{
    TestVectorGroup *group = new TestVectorGroup;
    group->next = m_groups;
    m_groups = group;
}

} // namespace gencrypto
