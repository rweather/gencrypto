gencrypto
=========

The "gencrypto" repository provides a framework for generating and
testing high performance assembly code versions of cryptography
primitives.  For reasons why this exists, see the "History" section below.

Building and Using
------------------

Gencrypto uses [cmake](https://cmake.org/) to build, so you will need to
have that installed.  Here is the simplest method to compile, test, and
generate algorithm code:

    mkdir build
    cd build
    cmake ..
    make
    make test
    make generate

The assembly output files will be placed into the "build/generated" directory.

Normally it will be necessary to adjust the function names and defines
to match your application's needs.  The files under the "templates"
directory provide the outer shell of each implementation.  Make a
new template by copying an existing one and then run the "gencrypto"
program as follows:

    build/src/gencrypto -t templates/my-ascon-avr5.txt test/vectors/ascon.txt
    build/src/gencrypto -o my-ascon-avr5.S templates/my-ascon-avr5.txt

The first command runs correctness tests on the code to verify its
functionality using gencrypto's built-in interpreter.  The second command
generates the assembly code output.

The templates may contain lines that are included conditionally, prefixed
with "%%if".  Conditional defines are be passed on the command-line as follows:

    build/src/gencrypto -Dcondition -o my-ascon-avr5.S templates/my-ascon-avr5.txt

History
-------

General-purpose compilers make a mess of cryptography code.  They generate
very poor code for a variety of reasons.  Obvious optimizations are not done.
Code is pushed out of line even when the programmer explicitly asked with
the "inline" keyword.

Available registers are not used because the compiler reserves some for
special cases.  The resulting code spills out to the stack more often than
it needs to.

Badly generated code may also leak information via side channels, even if
the programmer had taken care to write the code to mask leakages.

But writing assembly code by hand is hard!  The human mind stalls in the
face of the complexity.  Programmers usually solve this with assembly
macros to abstract the steps (s-box, diffusion, key update, etc).
But that can miss global optimizations that are possible with a
whole-of-algorithm approach.

These issues were very pronounced on AVR.  Standard AVR compilers do not deal
well with the 32-bit and 64-bit word rotation operations that are used in
modern algorithm designs.  AVR compilers tend to force those out of line
to a helper function which kills performance and leaks information.

This eventually drove me to devise the "genavr" tool in the
[Lightweight Cryptography](https://github.com/rweather/lightweight-crypto)
repository.  It has since evolved in my other LWC repositories.

The core idea of "genavr" is to provide a library of code generation and
register allocation primitives.  The programmer writes a C++ program that
mirrors the structure of the desired algorithm and the library takes care
of rendering that to code.  The programmer can provide hints at various
points like "must put this value in a high register" to guide the generation
of better code.  I have found that this makes it easier to spot global
optimisations.

During the second round of NIST's Lightweight Cryptography Competition (LWC),
this allowed me to write some of the world's fastest AVR implementations
of the submissions.  Most algorithms took no more than 2 or 3 days as the
library did most of the hard work of managing register allocations and
the translation of 32-bit and 64-bit word operations into 8-bit fragments.

A major advantage of "genavr" is that it includes an interpreter for the
AVR instruction set.  An algorithm's behaviour can be verified with the
interpreter before transferring the code to a target machine.
This accelerates the development time considerably.

Based on this success I made ARM Cortex M3 implementations of the
various LWC algorithms.  I wrote custom C code for each of the algorithms.
But I didn't make a common framework or interpreter this time.
This caused issues when trying to expand to other ARM and non-ARM
processors.  Expanding to masked ciphers also proved to be a problem.

The "gencrypto" repository is the end result of this evolution.
In addition to the AVR code generator there is a new code generator
for generating assembly versions of cryptopgraphy primitives on common
32-bit and 64-bit processors.

Contact
-------

For more information on this code, to report bugs, or to suggest
improvements, please contact the author Rhys Weatherley via
[email](mailto:rhys.weatherley@gmail.com).
