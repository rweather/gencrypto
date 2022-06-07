
enable_testing()

add_executable(gencrypto
    common/codegen.cpp
    common/codegen.h
    common/copyright.h
    common/insns.cpp
    common/insns.h
    common/main.cpp
    common/platform.cpp
    common/platform.h
    common/platform_arm.cpp
    common/platform_arm.h
    common/registry.cpp
    common/registry.h
    common/regs.cpp
    common/regs.h
    common/testvector.cpp
    common/testvector.h

    avr/code.cpp
    avr/code.h
    avr/code_out.cpp
    avr/interpret.cpp

    aes/aes-avr5.cpp

    ascon/ascon-avr5.cpp
    ascon/ascon-avr5-x2.cpp
    ascon/ascon-avr5-x3.cpp

    keccak/keccakp-200-avr5.cpp
    keccak/keccakp-400-avr5.cpp
    keccak/keccakp-1600-avr5.cpp

    sha256/sha256-avr5.cpp

    xoodoo/xoodoo-avr5.cpp
)
