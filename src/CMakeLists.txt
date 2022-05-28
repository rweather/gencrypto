
enable_testing()

add_executable(gencrypto
    common/codegen.h
    common/copyright.h
    common/main.cpp
    common/registry.cpp
    common/registry.h
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

    sha256/sha256-avr5.cpp

    xoodoo/xoodoo-avr5.cpp
)
