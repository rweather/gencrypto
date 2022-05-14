
enable_testing()

add_library(genavr STATIC
    avr/code.cpp
    avr/code.h
    avr/code_out.cpp
    avr/interpret.cpp
)

add_executable(ascon-avr
    ascon/ascon-avr5.h
    ascon/ascon-avr5.cpp
    ascon/ascon-avr5-x2.cpp
    ascon/ascon-avr5-x3.cpp
    ascon/ascon-avr5-main.cpp
)
target_link_libraries(ascon-avr PUBLIC genavr)
