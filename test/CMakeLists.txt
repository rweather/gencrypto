
enable_testing()

# Function to perform tests on a specific function.
function(alg_test family template)
    # Add a unit test to run the test.
    add_test(NAME ${template} COMMAND bash -c "${CMAKE_CURRENT_BINARY_DIR}/../src/gencrypto --test ${CMAKE_CURRENT_LIST_DIR}/../templates/${family}/${template}.txt ${CMAKE_CURRENT_LIST_DIR}/vectors/${family}.txt")

    # Add a custom command to generate the assembly code output.
    add_custom_command(
        OUTPUT ${template}
        COMMAND bash -c "${CMAKE_CURRENT_BINARY_DIR}/../src/gencrypto --output ${CMAKE_CURRENT_BINARY_DIR}/../generated/${template}.S ${CMAKE_CURRENT_LIST_DIR}/../templates/${family}/${template}.txt ${CMAKE_CURRENT_LIST_DIR}/vectors/${testvecs}.txt"
    )
    list(APPEND GENERATE_RULES ${template})
    set(GENERATE_RULES ${GENERATE_RULES} PARENT_SCOPE)
endfunction()

# Perform all of the tests.
alg_test(aes aes-avr5)
alg_test(ascon ascon-avr5)
alg_test(ascon ascon-avr5-x2)
alg_test(ascon ascon-avr5-x3)
alg_test(sha256 sha256-avr5)
alg_test(xoodoo xoodoo-avr5)

# Add a custom 'generate' target to generate all output files.
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/../generated)
add_custom_target(generate DEPENDS ${GENERATE_RULES})
