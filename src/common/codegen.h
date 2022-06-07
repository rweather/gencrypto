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

#ifndef GENCRYPTO_CODEGEN_H
#define GENCRYPTO_CODEGEN_H

#include "insns.h"
#include "platform.h"
#include <ostream>
#include <map>

namespace gencrypto
{

/**
 * \brief Code generator API.
 *
 * Subclasses implement code generation policies for different platforms.
 */
class CodeGenerator
{
    // Disable copy operators.
    CodeGenerator(const CodeGenerator &) {}
    CodeGenerator &operator=(const CodeGenerator &) { return *this; }
public:
    /**
     * \brief Construct a new code generator.
     *
     * \param platform The platform to generate code for; must not be NULL.
     */
    CodeGenerator(Platform *platform);

    /**
     * \brief Destroys this code generator.
     */
    ~CodeGenerator();

    /**
     * \brief Gets the platform that this object is generating code for.
     *
     * \return The platform.
     */
    Platform *platform() const { return m_platform; }

    /**
     * \brief Gets the stack pointer register for the platform.
     *
     * \return The stack pointer register.
     */
    Reg sp() const;

    /**
     * \brief Gets the size of registers that will be allocated from
     * the platform by default when allocateReg() is called.
     *
     * \return The allocation size; e.g. BasicRegister::Size32.
     */
    BasicRegister::Size allocationSize() const { return m_allocationSize; }

    /**
     * \brief Gets the size of registers that will be allocated from
     * the platform by default when allocateReg() is called.
     *
     * \param size The allocation size; e.g. BasicRegister::Size32.
     *
     * This may be needed when implementing a 32-bit algorithm on a
     * 64-bit platform to restrict the registers to their 32-bit variants.
     *
     * If \a size is not supported by any basic registers with the Data
     * property, then the request to change the size will be ignored.
     */
    void setAllocationSize(BasicRegister::Size size);

    /**
     * \brief Allocates a register of a specific bit size.
     *
     * \param size Size of the desired register in bits.
     * \param flags1 Flags that indicate the type of register that
     * we are looking for.  All flags must be present.
     * \param flags2 Backup flags if none could be found with \a flags1.
     * \param flags3 Backup flags if none could be found with \a flags2.
     * \param flags4 Backup flags if none could be found with \a flags3.
     *
     * If any of the flag arguments are zero, the zero flags will be ignored.
     *
     * \return The allocated register.
     */
    Reg allocateReg
        (size_t size, uint16_t flags1 = BasicRegister::Data,
         uint16_t flags2 = 0, uint16_t flags3 = 0, uint16_t flags4 = 0);

    /**
     * \brief Allocates a temporary register that we expect will be
     * released shortly within the same block.
     *
     * \param size Size of the desired temporary register in bits.
     *
     * \return The allocated temporary register.
     *
     * This will look for (Data | Temporary) first and then for just Data.
     */
    Reg allocateTempReg(size_t size)
        { return allocateReg
            (size, BasicRegister::Data | BasicRegister::Temporary,
             BasicRegister::Data); }

    /**
     * \brief Allocates a register for storage of values for later.
     *
     * \param size Size of the desired storage register in bits.
     *
     * \return The allocated storage register.
     *
     * This will look for Storage registers first and then for Data registers.
     * It is assumed that the returned register will not be involved in
     * arithmetic, logical, load, or store operations.  It is used to store
     * values temporarily in high registers without spilling them to the stack.
     */
    Reg allocateStorageReg(size_t size)
        { return allocateReg
            (size, BasicRegister::Storage, BasicRegister::Data); }

    /**
     * \brief Releases a register back to the allocation pool.
     *
     * \param reg The register to be released.  Will be replaced with a
     * null register on exit.
     */
    void releaseReg(Reg &reg);

    /**
     * \brief Types of arguments that may be passed to a function.
     */
    enum ArgType
    {
        ARG_INT8,           /**< Signed 8-bit integer */
        ARG_UINT8,          /**< Unsigned 8-bit integer */
        ARG_INT16,          /**< Signed 16-bit integer */
        ARG_UINT16,         /**< Unsigned 16-bit integer */
        ARG_INT32,          /**< Signed 32-bit integer */
        ARG_UINT32,         /**< Unsigned 32-bit integer */
        ARG_INT64,          /**< Signed 64-bit integer */
        ARG_UINT64,         /**< Unsigned 64-bit integer */
        ARG_PTR             /**< Pointer */
    };

    /**
     * \brief Adds an argument to the function that is being built.
     *
     * \param type The type of the next argument.
     *
     * \return A pre-allocated register that refers to the argument's value.
     *
     * The code should call releaseReg() when the argument value is no
     * longer required.
     *
     * This function must be called before any other code generation is
     * performed to ensure that the argument registers are not allocated
     * to some other purpose before the addArgument() call.
     *
     * If the argument is passed on the stack, then this function will
     * transfer the value to a register.
     */
    Reg addArgument(ArgType type);

    /**
     * \brief Sets up the local stack frame.
     *
     * \param sizeLocals Number of bytes of local variables to allocate.
     *
     * This function may allocate more than \a sizeLocals bytes to ensure
     * that the stack stays aligned on a platform-specific alignment.
     *
     * The local variables can be accessed using positive offsets from sp()
     * between 0 and \a sizeLocals - 1.
     */
    void setupLocals(size_t sizeLocals);

    /**
     * \brief Gets the number of bytes of local stack frame to allocate.
     *
     * \return The number of bytes for the local stack frame, which may
     * be rounded up from the value passed to setupLocals().
     */
    size_t localSize() const { return m_locals; }

    /**
     * \brief Sets up a function prologue for a permutation function.
     *
     * \param state Returns the argument register for the state pointer.
     * \param sizeLocals Number of bytes of local stack space to allocate.
     *
     * The resulting function will have the following prototype:
     *
     * \code
     * void permutation(void *state);
     * \endcode
     */
    void setupPermutation(Reg &state, size_t sizeLocals = 0);

    /**
     * \brief Sets up a function prologue for a permutation function that
     * takes an extra "count" parameter that indicates the number of rounds.
     *
     * \param state Returns the argument register for the state pointer.
     * \param count Returns the argument register for the count.
     * \param sizeLocals Number of bytes of local stack space to allocate.
     * \param type Type of the "count" parameter; default is ARG_UINT8.
     *
     * The resulting function will have the following prototype:
     *
     * \code
     * void permutation(void *state, uint8_t count);
     * \endcode
     */
    void setupPermutationWithCount
        (Reg &state, Reg &count, size_t sizeLocals = 0,
         ArgType type = ARG_UINT8);

    /**
     * \brief Adds an instruction to the list in this code generator.
     *
     * \param insn The instruction to add.
     */
    void addInsn(const Insn &insn) { m_insns.push_back(insn); }

    /**
     * \brief Re-schedule a previous instruction to move by an offset.
     *
     * \param offset The offset to apply, or zero for no offset.
     * \param index Index of the instruction from the end of the current
     * instruction list; 0 is the end of the list, 1 is one instruction
     * back, 2 is two instructions back, and so on.
     *
     * This can be used to give a hint to the code generator that the
     * instruction should be repositioned within the final output.
     *
     * The re-schedule request will be ignored if \a index is out of range.
     */
    void reschedule(int8_t offset, size_t index = 0);

#if 0
    void add(Reg &x, const Reg &y);
    void add(Reg &x, const Reg &y, const Reg &z);
    void add(Reg &x, uint64_t value);
    void add(Reg &x, const Reg &y, uint64_t value);
    void add_s(Reg &x, int64_t value);
    void add_s(Reg &x, const Reg &y, int64_t value);

    void logand(Reg &x, const Reg &y);
    void logand(Reg &x, const Reg &y, const Reg &z);
    void logand(Reg &x, uint64_t value);
    void logand(Reg &x, const Reg &y, uint64_t value);
    void logand_s(Reg &x, int64_t value);
    void logand_s(Reg &x, const Reg &y, int64_t value);

    void logand_not(Reg &x, const Reg &y);
    void logand_not(Reg &x, const Reg &y, const Reg &z);

    void lognot(Reg &x);
    void lognot(Reg &x, const Reg &y);

    void logor(Reg &x, const Reg &y);
    void logor(Reg &x, const Reg &y, const Reg &z);
    void logor(Reg &x, uint64_t value);
    void logor(Reg &x, const Reg &y, uint64_t value);
    void logor_s(Reg &x, int64_t value);
    void logor_s(Reg &x, const Reg &y, int64_t value);

    void logxor(Reg &x, const Reg &y);
    void logxor(Reg &x, const Reg &y, const Reg &z);
    void logxor(Reg &x, uint64_t value);
    void logxor(Reg &x, const Reg &y, uint64_t value);
    void logxor_s(Reg &x, int64_t value);
    void logxor_s(Reg &x, const Reg &y, int64_t value);

    void move(Reg &x, const Reg &y);
    void move(Reg &x, uint64_t value);
    void move_s(Reg &x, int64_t value);

    void neg(Reg &x);
    void neg(Reg &x, const Reg &y);

    void sub(Reg &x, const Reg &y);
    void sub(Reg &x, const Reg &y, const Reg &z);
    void sub(Reg &x, uint64_t value);
    void sub(Reg &x, const Reg &y, uint64_t value);
    void sub_s(Reg &x, int64_t value);
    void sub_s(Reg &x, const Reg &y, int64_t value);
#endif

private:
    typedef uint64_t RegMask;

    Platform *m_platform;
    std::vector<Insn> m_insns;
    BasicRegister::Size m_allocationSize;
    RegMask m_allocatedRegs;
    RegMask m_usedRegs;
    size_t m_nextArgumentReg;
    size_t m_nextArgumentOffset;
    size_t m_locals;

    Reg allocate(size_t size, uint16_t flags);
};

} // namespace gencrypto

#endif
