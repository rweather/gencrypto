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

#include "registry.h"
#include "copyright.h"
#include "testvector.h"
#include "avr/code.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <getopt.h>

#define short_options "c:D:lo:th"
static struct option long_options[] = {
    {"copyright",   required_argument,  0,  'c'},
    {"define",      required_argument,  0,  'D'},
    {"list",        no_argument,        0,  'l'},
    {"output",      required_argument,  0,  'o'},
    {"test",        no_argument,        0,  't'},
    {"help",        no_argument,        0,  'h'},
    {0,             0,                  0,    0}
};

static void usage(const char *progname)
{
    std::cerr << "Usage: " << progname
        << " [options] TEMPLATE [TEST-VECTORS]"
        << std::endl;
    std::cerr << std::endl;
    std::cerr << "    --copyright FILE, -c FILE" << std::endl;
    std::cerr << "        Use the contents of FILE for Copyright messages." << std::endl;
    std::cerr << std::endl;
    std::cerr << "    --define NAME, -D NAME" << std::endl;
    std::cerr << "        Define the option NAME." << std::endl;
    std::cerr << std::endl;
    std::cerr << "    --output FILE, -o FILE" << std::endl;
    std::cerr << "        Set the name of the output FILE, or '-' for standard output." << std::endl;
    std::cerr << std::endl;
    std::cerr << "    --list, -l" << std::endl;
    std::cerr << "        List all supported algorithms." << std::endl;
    std::cerr << std::endl;
    std::cerr << "    --test, -t" << std::endl;
    std::cerr << "        Run tests on the algorithms instead of generating code." << std::endl;
    std::cerr << std::endl;
    std::cerr << "    TEMPLATE" << std::endl;
    std::cerr << "        Name of the file containing the generator template." << std::endl;
    std::cerr << std::endl;
    std::cerr << "    TEST-VECTORS" << std::endl;
    std::cerr << "        Name of the file containing the test vectors for use with '--test'." << std::endl;
    std::cerr << std::endl;
}

static void listAlgorithms(std::ostream &out);
static bool generateAndRunTests
    (std::ostream &out, std::istream &templateFile, bool testMode,
     const gencrypto::TestVectorFile &tests,
     const std::vector<std::string> &options,
     const std::string &copyrightFilename);

int main(int argc, char *argv[])
{
    const char *progname = argv[0];
    std::vector<std::string> options;
    std::string outputFilename("-");
    std::string copyrightFilename;
    std::string templateFilename;
    std::string testVectorFilename;
    bool list = false;
    bool test = false;
    int opt;

    // Parse the command-line options.
    while ((opt = getopt_long(argc, argv, short_options, long_options, 0)) >= 0) {
        switch (opt) {
        case 'c':
            copyrightFilename = optarg;
            break;

        case 'D':
            options.push_back(optarg);
            break;

        case 'l':
            list = true;
            break;

        case 'o':
            outputFilename = optarg;
            break;

        case 't':
            test = true;
            break;

        case 'h':
        default:
            usage(progname);
            return 1;
        }
    }
    if (options.empty()) {
        options.push_back("default");
    }

    // Generation requires a template.  Testing also requires test vectors.
    std::ifstream templateFile;
    std::ifstream testVectorFile;
    if (!list) {
        if (optind >= argc) {
            usage(progname);
            return 1;
        }
        templateFilename = argv[optind];
        if (test && (optind + 1) >= argc) {
            usage(progname);
            return 1;
        }
        if (test) {
            testVectorFilename = argv[optind + 1];
        }
        templateFile.open(templateFilename);
        if (!templateFile.is_open()) {
            std::cerr << templateFilename
                      << ": could not open the template file"
                      << std::endl;
            return 1;
        }
        if (test) {
            testVectorFile.open(testVectorFilename);
            if (!templateFile.is_open()) {
                std::cerr << templateFilename
                          << ": could not open the test vector file"
                          << std::endl;
                return 1;
            }
        }
    }

    // Sort the list of registered algorithms.
    std::sort(gencrypto::Registration::registrations.begin(),
              gencrypto::Registration::registrations.end());

    // Open the output stream.
    std::ostream *out = &std::cout;
    std::ofstream file;
    if (outputFilename != "-") {
        file.open(outputFilename);
        if (!file.is_open()) {
            std::cerr << outputFilename
                      << ": could not open the output file"
                      << std::endl;
            return 1;
        }
        out = &file;
    }

    // Are we are listing the algorithms?
    if (list) {
        listAlgorithms(*out);
        return 0;
    }

    // Load the test vectors if necessary.
    gencrypto::TestVectorFile testVectors;
    if (test) {
        testVectors.load(testVectorFile);
        testVectorFile.close();
    }

    // Process the lines from the template and generate the output.
    // Alternatively, run tests for all function names in the template.
    if (generateAndRunTests
            (*out, templateFile, test, testVectors,
             options, copyrightFilename)) {
        return 0;
    } else {
        return 1;
    }
}

static void listAlgorithms(std::ostream &out)
{
    std::vector<gencrypto::Registration>::const_iterator it;
    for (it = gencrypto::Registration::registrations.cbegin();
            it != gencrypto::Registration::registrations.cend(); ++it) {
        out << it->qualifiedName();
        out << std::endl;
    }
}

static bool generateAndTestFunction
    (std::ostream &out, const gencrypto::Registration &info,
     bool testMode, const gencrypto::TestVectorFile &tests,
     const std::vector<std::string> &options)
{
    (void)options;
    if (info.generateAVR()) {
        AVR::Code code;
        info.generateAVR()(code);
        if (testMode && info.testAVR()) {
            gencrypto::TestVectorList vectors = tests.testsFor(info.name());
            gencrypto::TestVectorList::const_iterator it;
            bool ok = true;
            for (it = vectors.cbegin(); it != vectors.cend(); ++it) {
                out << info.qualifiedName() << "["
                    << it->name() << "] ... " << std::flush;
                if (info.testAVR()(code, *it)) {
                    out << "ok" << std::endl;
                } else {
                    out << "FAILED" << std::endl;
                    ok = false;
                }
            }
            return ok;
        } else if (!testMode) {
            if (code.size() != 0) {
                code.write(out);
            } else {
                // No code, but there may be S-boxes to write.
                unsigned count = code.sbox_count();
                for (unsigned index = 0; index < count; ++index) {
                    code.sbox_write(out, index, code.sbox_get(index));
                }
            }
        }
    } else if (info.generate()) {
        // TODO
    } else {
        std::cerr << "No generation function for '"
                  << info.qualifiedName() << "'" << std::endl;
        return false;
    }
    return true;
}

// Trim whitespace from the end of a string.
static std::string rtrim(const std::string &str)
{
    std::string temp(str);
    temp.erase(temp.find_last_not_of(" \t\r\n") + 1);
    return temp;
}

static bool generateAndRunTests
    (std::ostream &out, std::istream &templateFile, bool testMode,
     const gencrypto::TestVectorFile &tests,
     const std::vector<std::string> &options,
     const std::string &copyrightFilename)
{
    std::string line;
    int linenum = 0;
    bool success = true;
    bool skip;
    while (std::getline(templateFile, line)) {
        ++linenum;
        skip = false;
        line = rtrim(line);
        while (line.rfind("%%if(", 0) == 0) {
            // "%%if(option)" directive.  Check if the option is set.
            // There may be multiple conditions which are AND'ed together.
            size_t index = line.find("):");
            if (index == std::string::npos) {
                std::cerr << "line " << linenum
                          << ": invalid conditional '"
                          << line << "'" << std::endl;
                return false;
            }
            std::string option = line.substr(5, index - 5);
            line = line.substr(index + 2);
            if (std::find(options.begin(), options.end(), option)
                    == options.end()) {
                skip = true;
                break;
            }
        }
        if (skip) {
            // Conditional is false, so skip this line.
            continue;
        }
        if (line.size() >= 2 && line[0] == '%' && line[1] == '%') {
            // Process a directive in the template.
            if (line.rfind("%%copyright", 0) == 0) {
                // Output the Copyright notice here in generation mode.
                if (!testMode) {
                    if (copyrightFilename.empty()) {
                        out << copyright_message;
                    } else {
                        std::ifstream copyright;
                        copyright.open(copyrightFilename);
                        if (!copyright.is_open()) {
                            std::cerr << copyrightFilename
                                      << ": could not open the copyright file"
                                      << std::endl;
                            return false;
                        }
                        while (std::getline(copyright, line)) {
                            out << line << std::endl;
                        }
                        copyright.close();
                    }
                }
            } else if (line.rfind("%%function-body:", 0) == 0) {
                // Generate code for the function body and optionally test it.
                std::string name = line.substr(16);
                gencrypto::Registration info =
                    gencrypto::Registration::find(name);
                if (info.empty()) {
                    std::cerr << "line " << linenum << ": unknown function '"
                              << name << "'" << std::endl;
                    return false;
                } else if (!generateAndTestFunction
                        (out, info, testMode, tests, options)) {
                    if (!testMode) {
                        std::cerr << "line " << linenum << ": function '"
                                  << name << "' failed" << std::endl;
                        return false;
                    } else {
                        // When testing algorithms, test them all rather
                        // than stop at the first one.
                        success = false;
                    }
                }
            } else {
                std::cerr << "line " << linenum << ": unknown directive '"
                          << line << "'" << std::endl;
                return false;
            }
        } else if (!testMode) {
            // Copy this line as-is to the output.
            out << line << std::endl;
        }
    }
    return success;
}
