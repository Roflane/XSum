#include <fstream>
#include <utility>
#include <bit>
#include <array>
#include <string>
#include <cmath>

class checksum {
    using ChksmFnPtr = uint8_t (checksum::*)(uint8_t, size_t);

    static constexpr size_t BUFFER_SIZE = 4;
    static constexpr size_t PROCESS_COUNT = 16;

private:
    uint64_t _buffer[4] = {
        0x7FFFF737,
        0xDE930A10,
        0x83DC6440,
        0xAA837F10
    };
    char _bufferDynamic[4] = {};

    ChksmFnPtr _fnPtrs[BUFFER_SIZE] = {
        &checksum::_shr_xor,
        &checksum::_xor_and,
        &checksum::_shl_or,
        &checksum::_xor_shl_and_or,
    };

    const char* _fileName;
    std::ifstream _ifs;
    size_t _sizeBytes = 0;
    size_t _sizeBytesFixed = 0;

    uint8_t _shr_xor(uint8_t c, size_t offset) {
        uint8_t s = c & 0x3F;
        return (offset >> s) ^ c;
    }

    uint8_t _xor_and(uint8_t c, size_t offset) {
        return (offset ^ c) & c;
    }

    uint8_t _shl_or(uint8_t c, size_t offset) {
        uint8_t s = c & 7;
        return (c << s) | offset;
    }

    uint8_t _xor_shl_and_or(uint8_t c, size_t offset) {
        uint8_t s = c & 0x3F;
        return (offset << s) ^ (c & offset) | c;
    }

public:
    checksum(const char* fileName) : _fileName(fileName), _ifs(fileName, std::ios::binary) {
        char c;
        while (_ifs.get(c)) {
            ++_sizeBytes;
        }
        _ifs.clear();
        _ifs.seekg(0, std::ios::beg);
        _sizeBytesFixed = (_sizeBytes + BUFFER_SIZE - 1) / BUFFER_SIZE;
    }

    size_t getSizeFixed() const {
        return _sizeBytesFixed;
    }

    uint64_t sizeSeed() const {
        uint64_t x = _sizeBytes;
        x ^= x << 0x8;
        x ^= x >> 0xA;
        x ^= x << 0x4;
        return x;
    }
   
    std::string processBlocks() {
        std::string res = "";
        if (_sizeBytes < BUFFER_SIZE) return res;
        size_t fixedSize = getSizeFixed();
        uint64_t seed = sizeSeed();

        for (size_t i = 0; i < BUFFER_SIZE; ++i) {
            size_t offset = (fixedSize / BUFFER_SIZE) * i;
            std::string buffer = "";
            char v = 0;
            
            uint64_t localBuffers[BUFFER_SIZE];
            for (size_t i = 0; i < BUFFER_SIZE; ++i) {
                localBuffers[i] = _buffer[i] ^ (seed >> (i * 7));
                localBuffers[i] += (seed << (i + 3));
            }

            for (size_t j = 0; j < PROCESS_COUNT; ++j) {
                uint8_t v = (this->*_fnPtrs[i])(uint8_t(j + seed), offset);
                uint64_t mix = (v << (j + i + 1)) ^ offset >> (j % 7);

                uint64_t modifiedBuffer =
                    localBuffers[i] ^
                    (localBuffers[i] >> (i + 1)) ^
                    mix ^ (offset * offset + seed);

                modifiedBuffer = (modifiedBuffer << (v & 7)) |
                                 (modifiedBuffer >> (64 - (v & 7)));

                buffer += std::to_string(modifiedBuffer);
            } 
            buffer.insert(0, std::to_string(
                (seed ^ localBuffers[i]) & 0xFFFFFFFF
            ));
            res.append(buffer);
        }
        return res;
    }
};
