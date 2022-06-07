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

#ifndef GENCRYPTO_REGS_H
#define GENCRYPTO_REGS_H

#include <string>
#include <vector>
#include <cstdint>
#include <stdexcept>

namespace gencrypto
{

/**
 * \brief Information about a basic register on the platform.
 */
class BasicRegister
{
public:
    /**
     * \brief Constructs a new basic register.
     */
    BasicRegister() : p(0) {}

    /**
     * \brief Constructs a copy of another basic register.
     *
     * \param other The other basic register.
     */
    BasicRegister(const BasicRegister &other);

    /**
     * \brief Destroys this basic register.
     */
    ~BasicRegister();

    /**
     * \brief Sizes that are supported by this register.
     *
     * There may be multiple register names overlapping with each other
     * in the same physical register; e.g. "%al", "%ax", "%eax", and "%rax"
     * for x86-64 systems.
     */
    enum Size
    {
        Size8       = 8,        /**< 8-bit size available */
        Size16      = 16,       /**< 16-bit size available */
        Size32      = 32,       /**< 32-bit size available */
        Size64      = 64        /**< 64-bit size available */
    };

    /**
     * \brief Flags that indicate how the register can be used.
     */
    enum Flag
    {
        /** Can be used with two-address ALU instructions */
        TwoAddress          = 0x0001,

        /** Can be used with three-address ALU instructions */
        ThreeAddress        = 0x0002,

        /** Register is the stack pointer */
        StackPointer        = 0x0004,

        /** Register is the program counter */
        ProgramCounter      = 0x0008,

        /** Register is the link register for function call returns */
        Link                = 0x0010,

        /** Register can hold addresses to memory */
        Address             = 0x0020,

        /** Register can hold general-purpose data for ALU instructions */
        Data                = 0x0040,

        /** Register can be used for general storage without ALU support.
         *  Used for high registers or address registers on platforms
         *  that have the SplitRegisters feature. */
        Storage             = 0x0080,

        /** Writing a value to a smaller size sign-extends to the larger.
         *  Without this flag, smaller sizes zero-extend to larger. */
        SignExtend          = 0x0100,

        /** Register must be saved by the callee */
        CalleeSaved         = 0x0200,

        /** Register that is fixed to zero; usually combined with NoAllocate */
        Zero                = 0x0400,

        /** Register that may be destroyed by a jump; e.g. "ip" on ARM.
         *  Should only be used for temporary values within a block,
         *  and not for values whose lifetimes extend beyond a block. */
        Temporary           = 0x0800,

        /** Special register that cannot be allocated, such as "sp" or "pc" */
        NoAllocate          = 0x1000,
    };

    /**
     * \brief Assigns the contents of another basic register to this one.
     *
     * \param other The other basic register to assign to this one.
     *
     * \return A reference to this basic register.
     */
    BasicRegister &operator=(const BasicRegister &other);

    /**
     * \brief Gets the low-level register number for reference purposes.
     *
     * \return The low-level register number.
     */
    uint8_t number() const { return read()->number; }

    /**
     * \brief Sets the low-level register number for reference purposes.
     *
     * \param number The low-level register number.
     */
    void setNumber(uint8_t number) { write()->number = number; }

    /**
     * \brief Determine if no low-level register has been assigned
     * to this basic register.
     *
     * \return Returns true if this object is not assigned; false if it is.
     */
    bool isNull() const { return number() == 255; }

    /**
     * \brief Gets the sizes that are supported by this basic register.
     *
     * \param The supported sizes as a bitmask.
     */
    uint8_t sizes() const { return read()->sizes; }

    /**
     * \brief Sets the sizes that are supported by this basic register.
     *
     * \param sizes The supported sizes as a bitmask.
     */
    void setSizes(uint8_t sizes) { write()->sizes = sizes; }

    /**
     * \brief Determine if this basic register supports a specific size.
     *
     * \param size One of Size8, Size16, Size32, or Size64.
     *
     * \return Returns true if the basic register supports \a size;
     * false otherwise.
     */
    bool hasSize(Size size) const { return (read()->sizes & size) != 0; }

    /**
     * \brief Gets the maximum size supported by this basic register.
     *
     * \return One of Size8, Size16, Size32, or Size64.
     */
    Size maxSize() const;

    /**
     * \brief Gets the flags for features supported by this basic register.
     *
     * \param The feature flags for this basic register.
     */
    uint16_t flags() const { return read()->flags; }

    /**
     * \brief Sets the flags for features supported by this basic register.
     *
     * \param flags The feature flags for this basic register.
     */
    void setFlags(uint16_t flags) { write()->flags = flags; }

    /**
     * \brief Determine if this basic register has a specific feature.
     *
     * \param flag Flag for the feature.
     *
     * \return Returns true if this basic register has the feature;
     * false if not.
     */
    bool hasFlag(Flag flag) const { return (read()->flags & flag) == flag; }

    /**
     * \brief Gets the 8-bit name for this basic register.
     *
     * \return The 8-bit name, or empty if the basic register does not
     * support the 8-bit size.
     */
    std::string name8() const { return read()->name8; }

    /**
     * \brief Sets the 8-bit name for this basic register.
     *
     * \param name The 8-bit name.
     */
    void setName8(const std::string &name) { write()->name8 = name; }

    /**
     * \brief Gets the 16-bit name for this basic register.
     *
     * \return The 16-bit name, or empty if the basic register does not
     * support the 16-bit size.
     */
    std::string name16() const { return read()->name16; }

    /**
     * \brief Sets the 16-bit name for this basic register.
     *
     * \param name The 16-bit name.
     */
    void setName16(const std::string &name) { write()->name16 = name; }

    /**
     * \brief Gets the 32-bit name for this basic register.
     *
     * \return The 32-bit name, or empty if the basic register does not
     * support the 32-bit size.
     */
    std::string name32() const { return read()->name32; }

    /**
     * \brief Sets the 32-bit name for this basic register.
     *
     * \param name The 32-bit name.
     */
    void setName32(const std::string &name) { write()->name32 = name; }

    /**
     * \brief Gets the 64-bit name for this basic register.
     *
     * \return The 64-bit name, or empty if the basic register does not
     * support the 64-bit size.
     */
    std::string name64() const { return read()->name64; }

    /**
     * \brief Sets the 64-bit name for this basic register.
     *
     * \param name The 64-bit name.
     */
    void setName64(const std::string &name) { write()->name64 = name; }

    /**
     * \brief Gets the name of this basic register when it is used to
     * hold an address.
     *
     * \return The address name for this basic register.
     *
     * This is typically used for "32-bit on 64-bit" hosting situations
     * where the platform is pretending to be 32-bit but is actually
     * hosted on a 64-bit machine.  When a register is used for addressing,
     * the instruction must use the 64-bit form instead of the 32-bit form.
     *
     * If setAddressName() has not been called, then this function
     * will return one of name64(), name32(), or name16() depending
     * upon whichever is set.
     */
    std::string addressName() const;

    /**
     * \brief Sets the name of this basic register when it is used to
     * hold an address.
     *
     * \param name The address name for this basic register.
     */
    void setAddressName(const std::string &name) { write()->addrName = name; }

    /**
     * \brief Gets the size-appropriate name for this basic register.
     *
     * \param size The size to get the name for.
     *
     * \return The size-appropriate name, or an emtpy string if \a size
     * is not supported by this basic register.
     */
    std::string nameForSize(Size size) const;

    /**
     * \brief Constructs a 32-bit basic register that only has that size.
     *
     * \param number Low-level register number for the basic register.
     * \param name Name of the register.
     * \param flags Flags to associate with the register.
     *
     * \return The new basic register.
     *
     * This function is provided for convenience.
     */
    static BasicRegister reg32
        (uint8_t number, const std::string &name, uint16_t flags);

    /**
     * \brief Constructs a 64-bit basic register that only has that size.
     *
     * \param number Low-level register number for the basic register.
     * \param name Name of the register.
     * \param flags Flags to associate with the register.
     *
     * \return The new basic register.
     *
     * This function is provided for convenience.
     */
    static BasicRegister reg64
        (uint8_t number, const std::string &name, uint16_t flags);

    /**
     * \brief Constructs a basic register that has both 32-bit and
     * 64-bit variants.
     *
     * \param number Low-level register number for the basic register.
     * \param name32 Name of the register in its 32-bit variant.
     * \param name64 Name of the register in its 64-bit variant.
     * \param flags Flags to associate with the register.
     *
     * \return The new basic register.
     *
     * This function is provided for convenience.
     */
    static BasicRegister reg3264
        (uint8_t number, const std::string &name32,
         const std::string &name64, uint16_t flags);

private:
    struct Private
    {
        uint32_t refCount;
        uint8_t number;
        uint8_t sizes;
        uint16_t flags;
        std::string name8;
        std::string name16;
        std::string name32;
        std::string name64;
        std::string addrName;

        Private();
        Private(const Private &other);
    };
    mutable Private *p;

    const Private *read() const;
    Private *write();
};

/**
 * \brief Basic register that has been decorated with its chosen size.
 */
class SizedRegister
{
public:
    /**
     * \brief Constructs an empty sized register.
     */
    SizedRegister() : m_size(BasicRegister::Size8) {}

    /**
     * \brief Constructs a sized register from a basic register and size.
     *
     * \param reg The basic register.
     * \param size The size to assign to the register, which must be
     * one of the supported sizes.
     *
     * Throws an exception if \a size is not one of the supported sizes.
     */
    SizedRegister(const BasicRegister &reg, BasicRegister::Size size);

    /**
     * \brief Constructs a copy of another sized register.
     *
     * \param other The other sized register to copy.
     */
    SizedRegister(const SizedRegister &other)
        : m_reg(other.m_reg), m_size(other.m_size) {}

    /**
     * \brief Destroys this sized register.
     */
    ~SizedRegister() {}

    /**
     * \brief Assigns a copy of another sized register to this one.
     *
     * \param other The other sized register to assign.
     *
     * \return A reference to this sized register.
     */
    SizedRegister &operator=(const SizedRegister &other)
    {
        if (&other != this) {
            m_reg = other.m_reg;
            m_size = other.m_size;
        }
        return *this;
    }

    /**
     * \brief Gets the basic register that underlies this sized register.
     *
     * \return The basic register.
     */
    BasicRegister reg() const { return m_reg; }

    /**
     * \brief Gets the size that has been assigned to the register.
     *
     * \return The size of the basic register that has been assigned.
     */
    BasicRegister::Size size() const { return m_size; }

    /**
     * \brief Gets the size-appropriate name of the basic register.
     *
     * \return The name of the basic register.
     */
    std::string name() const { return m_reg.nameForSize(m_size); }

    /**
     * \brief Gets the low-level register number for reference purposes.
     *
     * \return The low-level register number.
     */
    uint8_t number() const { return m_reg.number(); }

    /**
     * \brief Determine if no low-level register has been assigned
     * to this sized register.
     *
     * \return Returns true if this object is not assigned; false if it is.
     */
    bool isNull() const { return m_reg.isNull(); }

    /**
     * \brief Determine if two sized register objects refer to the
     * same underlying basic register.
     *
     * \param other The other sized register to compare against.
     *
     * \return Returns true if this sized register has the same basic
     * register and size as \a other; false otherwise.
     */
    bool operator==(const SizedRegister &other) const
    {
        return m_reg.number() == other.m_reg.number() &&
               m_size == other.m_size;
    }

    /**
     * \brief Determine if two sized register objects do not refer to the
     * same underlying basic register.
     *
     * \param other The other sized register to compare against.
     *
     * \return Returns false if this sized register has the same basic
     * register and size as \a other; true otherwise.
     */
    bool operator!=(const SizedRegister &other) const
    {
        return m_reg.number() != other.m_reg.number() ||
               m_size != other.m_size;
    }

private:
    BasicRegister m_reg;
    BasicRegister::Size m_size;
};

/**
 * \brief Representation of a arbitrary-sized register value that is stored
 * in one or more basic registers.
 *
 * Instances of this class do not need to be multiples of 32 bits or
 * 64 bits or whatever the native word size of the platform is.  It is
 * thus possible to create an 8-bit, 16-bit, 32-bit, 59-bit, or even a
 * 257-bit register on both 32-bit and 64-bit platforms.
 *
 * The code generator ensures that any left-over high bits will be treated
 * as zero during operations.  The generator may ensure this by explicitly
 * setting the high bits to zero during previous operations.  Or it may allow
 * garbage in the high bits that is masked off in the following operations.
 * The zeroFill() property indicates if the bits are known to be zero.
 */
class Reg
{
public:
    /**
     * \brief Constructs an empty register with zero bits.
     */
    Reg();

    /**
     * \brief Constructs a copy of another register.
     *
     * \param The other register to copy.
     */
    Reg(const Reg &other);

    /**
     * \brief Constructs a register from a single sized register.
     *
     * \param reg The sized register.
     */
    Reg(const SizedRegister &reg);

    /**
     * \brief Constructs a register from a basic register in its maximum size.
     *
     * \param reg The basic register.
     */
    Reg(const BasicRegister &reg);

    /**
     * \brief Constructs a register from a basic register in a specific size.
     *
     * \param reg The basic register.
     * \param size The basic register size to use.
     */
    Reg(const BasicRegister &reg, BasicRegister::Size size);

    /**
     * \brief Destroys this register.
     */
    ~Reg() {}

    /**
     * \brief Assigns a copy of another register to this one.
     *
     * \param The other register to assign to this one.
     *
     * \return A reference to this register.
     */
    Reg &operator=(const Reg &other);

    /**
     * \brief Gets the size of the value in this register in bits (not bytes).
     *
     * \return The size of this register in bits.
     */
    size_t size() const { return m_size; }

    /**
     * \brief Sets the size of the value in this register in bits (not bytes).
     *
     * \param size The size of this register in bits.
     *
     * The supplied \a size must be less than or equal to fullSize()
     * and must also be within the extent of the most significant
     * basic register.  An exception will be thrown if this is not the case.
     */
    void setSize(size_t size);

    /**
     * \brief Determine if this register is null / empty.
     *
     * \return Returns true if this register has no members.
     */
    bool isNull() const { return m_size == 0; }

    /**
     * \brief Gets the full size of this register in bits, including
     * any left-over bits beyond size().
     *
     * \return The full size of this register in bits.
     */
    size_t fullSize() const { return m_fullSize; }

    /**
     * \brief Size of a single sized register within this larger register.
     *
     * \param The size of a single sized register in bits, or zero if there
     * are no sized registers in this object yet.
     */
    size_t limbSize() const;

    /**
     * \brief Determine if the unused high bits can be assumed to be zero.
     *
     * \return Returns true if the high bits beyond size() can be assumed
     * to be zero; or false if the high bits contain unknown garbage.
     */
    bool zeroFill() const { return m_zeroFill; }

    /**
     * \brief Set the flag that indicates if the unused high bits can be
     * assumed to be zero.
     *
     * \param zeroFill Set to true if the high bits beyond size() can be
     * assumed to be zero; or false if the high bits contain unknown garbage.
     */
    void setZeroFill(bool zeroFill) { m_zeroFill = zeroFill; }

    /**
     * \brief Adds a basic register to this register with a specific size.
     *
     * \param reg The basic register to add.
     * \param size The size of the basic register to use if it has
     * multiple sizes.
     *
     * Registers should be added from least significant to most significant.
     *
     * Use setSize() to restrict the size of the most significant register
     * to less than the natively-supported \a size.
     */
    void addRegister(const BasicRegister &reg, BasicRegister::Size size)
        { addRegister(SizedRegister(reg, size)); }

    /**
     * \brief Adds a basic register to this register with its maximum word size.
     *
     * \param reg The basic register to add.
     */
    void addRegister(const BasicRegister &reg)
        { addRegister(SizedRegister(reg, reg.maxSize())); }

    /**
     * \brief Adds a pre-sized basic register to this register.
     *
     * \param reg The pre-sized basic register to add.
     *
     * If \a reg already appears in the register or it does not have the
     * same size as existing registers, an exception will be thrown.
     */
    void addRegister(const SizedRegister &reg);

    /**
     * \brief Gets the number of sized regsiters within this register.
     *
     * \return The number of sized registers. 
     */
    size_t numRegs() const { return m_regs.size(); }

    /**
     * \brief Gets a specific sized register from within this register.
     *
     * \param index The index of the sized register between 0 and numRegs() - 1.
     *
     * \return The sized register at \a index.
     */
    SizedRegister reg(size_t index) const { return m_regs[index]; }

    /**
     * \brief Gets a specific basic register from within this register.
     *
     * \param index The index of the basic register between 0 and numRegs() - 1.
     *
     * \return The basic register at \a index.
     */
    BasicRegister basicReg(size_t index) const { return m_regs[index].reg(); }

    /**
     * \brief Gets the platform-specific register number for a basic register.
     *
     * \param index The index of the basic register between 0 and numRegs() - 1.
     *
     * \return The platform-specific register number.
     */
    uint8_t number(size_t index) const { return m_regs[index].reg().number(); }

    /**
     * \brief Gets the size-appropriate name of a specific sized register.
     *
     * \param index The index of the sized register between 0 and numRegs() - 1.
     *
     * \return The name of the sized register at \a index.
     */
    std::string name(size_t index) const { return m_regs[index].name(); }

    /**
     * \brief Gets the reversed version of this register where the
     * individual registers are rearranged.
     *
     * \return The reversed version of this register.
     *
     * Will throw an exception if size() is not a multiple of the basic
     * register size.
     */
    Reg reversed() const;

    /**
     * \brief Gets a subset of this register.
     *
     * \param start The starting bit which must be a multiple of the
     * basic register size.
     * \param len The length of the bit range, or 0 for everything
     * from \a start to the end of the register.
     *
     * \return A new register that refers to the subset.
     *
     * Will throw an exception if the arguments are not acceptable.
     */
    Reg subset(size_t start, size_t len) const;

    /**
     * \brief Gets a subset of this register starting at a specific point.
     *
     * \param start The starting bit which must be a multiple of the
     * basic register size.
     *
     * \return A new register that refers to the subset starting at \a start.
     *
     * Will throw an exception if \a start is not a multiple of the
     * basic register size.
     */
    Reg subset(size_t start) const { return subset(start, 0); }

    /**
     * \brief Reduces this register to a smaller number of bits.
     *
     * \param len The length in bits to reduce the register to.
     *
     * \return A new register that refers to the reduced version,
     * containing the \a len least significant bits of this register.
     */
    Reg reduce(size_t len) const { return subset(0, len); }

private:
    size_t m_size;
    size_t m_fullSize;
    bool m_zeroFill;
    std::vector<SizedRegister> m_regs;
};

} // namespace gencrypto

#endif
