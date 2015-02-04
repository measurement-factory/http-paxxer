/* Copyright (c) 2015, The Measurement Factory. */

#include <iostream>
#include <sstream>
#include <limits>
#include <cassert>
#include <cstdlib>
#include <stdint.h>

/* Based on HPAC draft-ietf-httpbis-header-compression-10, Section 5.1 */

// XXX: The interface will change.
// TODO: Transform into Connection::sendInteger().
static
void sendInteger(const uint64_t value, const int prefixSize)
{
    /*
        if I < 2^N - 1, encode I on N bits
        else
            encode (2^N - 1) on N bits
            I = I - (2^N - 1)
            while I >= 128
                encode (I % 128 + 128) on 8 bits
                I = I / 128
            encode I on 8 bits
    */

    assert(1 <= prefixSize && prefixSize <= 8);
    const uint8_t maxPrefix = (1 << prefixSize) - 1;

    // TODO: The largest integer we are going to support will fit into
    // less than 128 bits. Moreover, integer packing is going to be an atomic
    // operation. Optimize using a fixed-size on-stack byte array.
    std::string res;
    if (value < maxPrefix) {
        res.push_back(static_cast<uint8_t>(value));
    } else {
        res.push_back(static_cast<uint8_t>(maxPrefix));
        uint64_t remainder = value - maxPrefix; // "I" in HPAC terms
        while (remainder >= 128) {
            const uint8_t byte = (remainder % 128) + 128;
            res.push_back(byte);
            remainder /= 128;
        }
        res.push_back(static_cast<uint8_t>(remainder));
    }
    std::cout << res;
    std::cout.flush();
}

// XXX: The interface will change.
// TODO: Transform inner code into Connection::receiveInteger().
static
void receiveIntegers(const char *buffer, const size_t size, const int prefixSize)
{
    /*
        decode I from the next N bits
        if I < 2^N - 1, return I
        else
            M = 0
            repeat
                B = next octet
                I = I + (B & 127) * 2^M
                M = M + 7
            while B & 128 == 128
        return I
    */

    // TODO: Remove this check after isolating the single-integer parser.
    // We should check how that parser handles empty input,
    // not protect that parser from it.
    if (!size) {
        std::cerr << "encoding buffer is empty" << std::endl;
        exit(-2);
    }

    assert(1 <= prefixSize && prefixSize <= 8);
    const uint8_t maxPrefix = (1 << prefixSize) - 1;
    const uint64_t maxValue = std::numeric_limits<uint64_t>::max();

    size_t bytePos = 0;
    while (bytePos < size) {
        uint64_t value = buffer[bytePos++] & maxPrefix; // "I" in HPAC terms
        if (value >= maxPrefix) {
            unsigned int exponent = 0; // "M" in HPAC terms
            do {
                if (bytePos >= size) {
                    std::cerr << "premature end of integer encoding at byte " << bytePos << std::endl;
                    exit(-2);
                }

                const uint8_t byte = buffer[bytePos++]; // "B" in HPAC terms

                if (const uint64_t rawDelta = byte & 127) {
                    const uint64_t delta = rawDelta << exponent;
                    if ((delta >> exponent) != rawDelta) {
                        std::cerr << "value increment overflow at byte " << (bytePos-1) << " of encoding buffer" << std::endl;
                        exit(-2);
                    }
                    if (delta > maxValue-value) {
                        std::cerr << "value overflow at byte " << (bytePos-1) << " of encoding buffer" << std::endl;
                        exit(-2);
                    }
                    value += delta;
                }

                if ((byte & 128) == 0)
                    break;

                exponent += 7;
                // protect us from m overflows, stopping at the first m value
                // too big for the rawDelta shift above
                if (exponent >= 64) {
                    std::cerr << "weight factor overflow at byte " << (bytePos-1) << " of encoding buffer" << std::endl;
                    exit(-2);
                }
            } while (true);
        }
        std::cout << value << std::endl;
        std::cout.flush();
    }
}

static
void printUsage(std::ostream &os, const std::string &progName)
{
    os << "Usage: " << progName << " <action> [N]" << std::endl;
    os << "supported actions:" << std::endl;
    os << "  --parse  read integers in wire format, write one parsed integer per line" << std::endl;
    os << "  --pack   read human-formatted unsigned integers, write them in wire format" << std::endl;
    os << "  --help   print this message" << std::endl;
    os << "    N      wire format prefix size; valid values are 1 through 8" << std::endl;
}

int main(int argc, char *argv[])
{
    if (argc < 2 || argc > 3) {
        std::cerr << "error: wrong number of arguments" << std::endl;
        printUsage(std::cerr, argv[0]);
        return -1;
    }

    int prefixSize = 5;
    if (argc == 3) {
        const std::string arg2(argv[2]);
        std::stringstream ss(arg2);
        ss >> prefixSize;
        if (!(1 <= prefixSize && prefixSize <= 8)) {
            std::cerr << "error: unsupported prefix size value: " << arg2 << std::endl;
            printUsage(std::cerr, argv[0]);
            return -1;
        }
    }

    const std::string arg1(argv[1]);
    if (arg1 == "--pack") {
        uint64_t number;
        // do not be picky about input; eat whitespace between numbers
        while (std::cin >> number)
            sendInteger(number, prefixSize);
    } else
    if (arg1 == "--parse") {
        // slow but short (and too smart?)
        typedef std::istreambuf_iterator<char> SISBIC;
        const std::string raw((SISBIC(std::cin)), SISBIC());
        receiveIntegers(raw.data(), raw.length(), prefixSize);
    } else
    if (arg1 == "--help") {
        printUsage(std::cout, argv[0]);
    } else {
        std::cerr << "error: unsupported action: " << arg1 << std::endl;
        printUsage(std::cerr, argv[0]);
        return -1;
    }

    return 0;
}
