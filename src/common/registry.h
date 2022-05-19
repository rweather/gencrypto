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

#ifndef GENCRYPTO_REGISTRY_H
#define GENCRYPTO_REGISTRY_H

#include "testvector.h"

namespace AVR
{

class Code;

};

namespace gencrypto
{

class CodeGenerator;

/**
 * \brief Prototype for a handler that generates the code for a function.
 *
 * \param code Code generator to use.
 */
typedef void (*GenerateHandler_t)(CodeGenerator &code);

/**
 * \brief Prototype for a handler that runs tests on a function.
 *
 * \param code Code generator that contains the code that was
 * previously generated.
 * \param vec Test vector to run against the function.  May be empty
 * if the test is standalone without any test vectors.
 *
 * \return Returns true if the tests were successful or false on failure.
 */
typedef bool (*TestHandler_t)(CodeGenerator &code, const TestVector &vec);

// AVR platforms have specialised requirements, so we need different generators.

/**
 * \brief Prototype for a handler that generates AVR code for a function.
 *
 * \param code Code generator to use.
 */
typedef void (*AVRGenerateHandler_t)(AVR::Code &code);

/**
 * \brief Prototype for a handler that runs tests on a function.
 *
 * \param code Code generator that contains the code that was
 * previously generated.
 * \param vec Test vector to run against the function.  May be empty
 * if the test is standalone without any test vectors.
 *
 * \return Returns true if the tests were successful or false on failure.
 */
typedef bool (*AVRTestHandler_t)(AVR::Code &code, const TestVector &vec);

/**
 * \brief Information about a registered function code generator.
 */
class Registration
{
public:
    /**
     * \brief Constructs an empty registration object.
     */
    Registration() : m_generate(0), m_test(0), m_generateAVR(0), m_testAVR(0) {}

    /**
     * \brief Constructs a copy of another registration object.
     *
     * \param other The other registration object.
     */
    Registration(const Registration &other);

    /**
     * \brief Destroys this registration object.
     */
    ~Registration() {}

    /**
     * \brief Assigns another registration object to this one.
     *
     * \param other The other registration object.
     *
     * \return A reference to this registration object.
     */
    Registration &operator=(const Registration &other);

    /**
     * \brief Determine if this registration is empty and does not
     * refer to a function.
     *
     * \return Returns true if this registration is empty.
     */
    bool empty() const { return m_name.empty(); }

    /**
     * \brief Gets the name of the function.
     *
     * \return The name of the function.
     */
    std::string name() const { return m_name; }

    /**
     * \brief Sets the name of the function.
     *
     * \param name The name of the function.
     */
    void setName(const std::string &name) { m_name = name; }

    /**
     * \brief Gets the variant of the function, or an empty string
     * if there is no variant.
     *
     * \return The variant name.
     */
    std::string variant() const { return m_variant; }

    /**
     * \brief Sets the variant of the function.
     *
     * \param variant The variant name.
     */
    void setVariant(const std::string &variant) { m_variant = variant; }

    /**
     * \brief Gets the name of the platform that the function will run on.
     *
     * \return The name of the platform.
     */
    std::string platform() const { return m_platform; }

    /**
     * \brief Sets the name of the platform that the function will run on.
     *
     * \param platform The name of the platform.
     */
    void setPlatform(const std::string &platform) { m_platform = platform; }

    /**
     * \brief Gets the fully qualified name of the function, combining
     * name(), variant(), and platform().
     *
     * \return The fully qualified name.
     */
    std::string qualifiedName() const;

    /**
     * \brief Gets the code generation handler for the function.
     *
     * \return The code generation handler, or NULL.
     */
    GenerateHandler_t generate() const { return m_generate; }

    /**
     * \brief Sets the code generation handler for the function.
     *
     * \param generate The code generation handler, or NULL.
     */
    void setGenerate(GenerateHandler_t generate) { m_generate = generate; }

    /**
     * \brief Gets the test handler for the function.
     *
     * \return The test handler, or NULL.
     */
    TestHandler_t test() const { return m_test; }

    /**
     * \brief Sets the test handler for the function.
     *
     * \param test The test handler, or NULL.
     */
    void setTest(TestHandler_t test) { m_test = test; }

    /**
     * \brief Gets the AVR code generation handler for the function.
     *
     * \return The AVR code generation handler, or NULL.
     */
    AVRGenerateHandler_t generateAVR() const { return m_generateAVR; }

    /**
     * \brief Sets the AVR code generation handler for the function.
     *
     * \param generate The AVR code generation handler, or NULL.
     */
    void setGenerateAVR(AVRGenerateHandler_t generate) { m_generateAVR = generate; }

    /**
     * \brief Gets the AVR test handler for the function.
     *
     * \return The AVR test handler, or NULL.
     */
    AVRTestHandler_t testAVR() const { return m_testAVR; }

    /**
     * \brief Sets the AVR test handler for the function.
     *
     * \param test The AVR test handler, or NULL.
     */
    void setTestAVR(AVRTestHandler_t test) { m_testAVR = test; }

    /**
     * \brief List of all registrations in the system.
     *
     * \sa GENCRYPTO_REGISTER(), GENCRYPTO_REGISTER_AVR()
     */
    static std::vector<Registration> registrations;

    /**
     * \brief Registers a function code generator at run time.
     *
     * \param name Name of the function.
     * \param variant Options that describe the specific variant, or NULL.
     * \param platform Name of the platform that the function generates
     * code for.
     * \param gen Points to the handler to use to generate the function.
     * \param test Points to the handler to use to test the function,
     * or NULL if testing is not supported.
     *
     * \sa GENCRYPTO_REGISTER()
     */
    static void registerFunction
        (const char *name, const char *variant, const char *platform,
         GenerateHandler_t gen, TestHandler_t test);

    /**
     * \brief Registers an AVR function code generator at run time.
     *
     * \param name Name of the function.
     * \param variant Options that describe the specific variant, or NULL.
     * \param platform Name of the platform that the function generates
     * code for, usually "avr5".
     * \param gen Points to the handler to use to generate the function.
     * \param test Points to the handler to use to test the function,
     * or NULL if testing is not supported.
     *
     * \sa GENCRYPTO_REGISTER_AVR()
     */
    static void registerFunctionAVR
        (const char *name, const char *variant, const char *platform,
         AVRGenerateHandler_t gen, AVRTestHandler_t test);

    /**
     * \brief Compares this registration with another for sorting.
     *
     * \param other The other registration to compare against.
     *
     * \return Returns true if this registration is less than \a other.
     */
    bool operator<(const Registration &other) const;

    /**
     * \brief Finds the registration for a function with a specific name.
     *
     * \param qualifiedName The fully qualified name for the function
     * including the variant and platform.
     *
     * \return The registration that was found.
     */
    static Registration find(const std::string &qualifiedName);

private:
    std::string m_name;
    std::string m_variant;
    std::string m_platform;
    GenerateHandler_t m_generate;
    TestHandler_t m_test;
    AVRGenerateHandler_t m_generateAVR;
    AVRTestHandler_t m_testAVR;
};

/**
 * \brief Registers a function code generator at compile time.
 *
 * \param name Name of the function.
 * \param variant Options that describe the specific variant, or NULL.
 * \param platform Name of the platform that the function generates code for.
 * \param gen Points to the handler to use to generate the function.
 * \param test Points to the handler to use to test the function,
 * or NULL if testing is not supported.
 *
 * For example:
 *
 * \code
 * GENCRYPTO_REGISTER("ascon_x3_permute", "3shares", "armv7m",
 *                    gen_arm_ascon_x3_3_permutation,
 *                    test_arm_ascon_x3_3_permutation);
 * GENCRYPTO_REGISTER("ascon_x3_permute", "4shares", "armv7m",
 *                    gen_arm_ascon_x3_4_permutation,
 *                    test_arm_ascon_x3_4_permutation);
 * GENCRYPTO_REGISTER("ascon_x4_permute", 0, "armv7m",
 *                    gen_arm_ascon_x4_permutation,
 *                    test_arm_ascon_x4_permutation);
 * \endcode
 */
#define GENCRYPTO_REGISTER(name, variant, platform, gen, test) \
    class reg_##gen { \
    private: \
        int x; \
    public: \
        inline reg_##gen() : x(0) \
            { gencrypto::Registration::registerFunction \
                    ((name), (variant), (platform), (gen), (test)); }; \
    }; \
    reg_##gen reg_instance_##gen

/**
 * \brief Registers an AVR function code generator at compile time.
 *
 * \param name Name of the function.
 * \param variant Options that describe the specific variant, or NULL.
 * \param platform Name of the platform that the function generates code for,
 * usually "avr5".
 * \param gen Points to the handler to use to generate the function.
 * \param test Points to the handler to use to test the function,
 * or NULL if testing is not supported.
 *
 * For example:
 *
 * \code
 * GENCRYPTO_REGISTER_AVR("ascon_x2_permute", "2shares", "avr5",
 *                        gen_avr_ascon_x2_2_permutation,
 *                        test_avr_ascon_x2_2_permutation);
 * GENCRYPTO_REGISTER_AVR("ascon_x2_permute", "3shares", "avr5",
 *                        gen_avr_ascon_x2_3_permutation,
 *                        test_avr_ascon_x2_3_permutation);
 * GENCRYPTO_REGISTER_AVR("ascon_x3_permute", 0, "avr5",
 *                        gen_avr_ascon_x3_permutation,
 *                        test_avr_ascon_x3_permutation);
 * \endcode
 */
#define GENCRYPTO_REGISTER_AVR(name, variant, platform, gen, test) \
    class reg_##gen { \
    private: \
        int x; \
    public: \
        inline reg_##gen() : x(0) \
            { gencrypto::Registration::registerFunctionAVR \
                    ((name), (variant), (platform), (gen), (test)); }; \
    }; \
    reg_##gen reg_instance_##gen

} // namespace gencrypto

#endif
