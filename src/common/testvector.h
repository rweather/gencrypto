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

#ifndef GENCRYPTO_TESTVECTOR_H
#define GENCRYPTO_TESTVECTOR_H

#include <string>
#include <vector>
#include <map>
#include <istream>

namespace gencrypto
{

/**
 * \brief Storage for a test vector.
 */
class TestVector
{
public:
    /**
     * \brief Constructs a test vector.
     */
    TestVector() {}

    /**
     * \brief Constructs a copy of another test vector.
     *
     * \param other The other test vector.
     */
    TestVector(const TestVector &other) : m_map(other.m_map) {}

    /**
     * \brief Destroys this test vector.
     */
    ~TestVector() {}

    /**
     * \brief Assigns a copy of another test vector to this one.
     *
     * \param other The other test vector.
     *
     * \return A reference to this test vector.
     */
    TestVector& operator=(const TestVector &other)
    {
        m_map = other.m_map;
        return *this;
    }

    /**
     * \brief Inserts a key / value pair into this test vector.
     *
     * \param key The key to insert as.
     * \param value The value to insert.
     */
    void insert(const std::string &key, const std::string &value)
    {
        m_map.insert(std::pair<std::string, std::string>(key, value));
    }

    /**
     * \brief Gets the name of the test vector.
     *
     * \return The test vector name.
     */
    std::string name() const { return valueAsString("Name"); }

    /**
     * \brief Gets a value from this vector as a string.
     *
     * \param key The key to look up.
     *
     * \return The string value of \a key, or an empty string otherwise.
     */
    std::string valueAsString(const std::string &key) const;

    /**
     * \brief Gets a value from this vector after converting from hex to binary.
     *
     * \param key The key to look up.
     *
     * \return The binary vector after conversion from hex.
     */
    std::vector<unsigned char> valueAsBinary(const std::string &key) const;

    /**
     * \brief Gets a value from this test vector after converting to an integer.
     *
     * \param key The key to look up.
     * \param defaultValue The default value to return if \a key is not
     * present in the test vector.
     *
     * \return The integer value.
     */
    int valueAsInt(const std::string &key, int defaultValue = -1) const;

    /**
     * \brief Determine if this test vector is empty.
     *
     * \return Returns true if empty, or false if not.
     */
    bool empty() const { return m_map.empty(); }

    /**
     * \brief Populates an array of bytes with a value from this test vector.
     *
     * \param buf Points to the buffer.
     * \param size Size of the buffer.
     * \param name Name of the field in the test vector.
     *
     * \return Returns true if \a buf was populated or false if the
     * field does not exist or is the wrong size.
     */
    bool populate(unsigned char *buf, size_t size, const char *name) const;

    /**
     * \brief Checks an array of bytes against a value from this test vector.
     *
     * \param buf Points to the buffer.
     * \param size Size of the buffer.
     * \param name Name of the field in the test vector.
     *
     * \return Returns true if the contents of \a buf are the same as
     * the contents of the field \a name.
     */
    bool check(const unsigned char *buf, size_t size, const char *name) const;

private:
    std::map<std::string, std::string> m_map;
};

/**
 * \brief List of test vectors.
 */
typedef std::vector<TestVector> TestVectorList;

/**
 * \brief Contents of a file containing test vectors.
 */
class TestVectorFile
{
    // Disable the copy operators.
    TestVectorFile(const TestVectorFile &) {}
    TestVectorFile &operator=(const TestVectorFile &) { return *this; }
public:
    /**
     * \brief Constructos a new test vector file.
     */
    TestVectorFile();

    /**
     * \brief Destroys this test vector file.
     */
    ~TestVectorFile();

    /**
     * \brief Loads the contents of a test vector file.
     *
     * \param file The stream for the open file.
     */
    void load(std::istream &file);

    /**
     * \brief Gets the tests for a specific function name.
     *
     * \param name Name of the function to look for.
     *
     * \return The list of tests to apply to the function.
     */
    TestVectorList testsFor(const std::string &name) const;

private:
    struct TestVectorGroup
    {
        std::vector<std::string> names;
        TestVectorList vectors;
        TestVectorGroup *next;
    };
    TestVectorGroup *m_groups;

    void clear();
    void addGroup();
};

} // namespace gencrypto

#endif
