/**
 * HYPERVISOR CHEAT - XOR String Encryption
 * Compile-time string encryption for anti-detection
 * 
 * Usage: xorstr_("string") or _XS("string")
 */

#pragma once

#include <cstdint>
#include <utility>

namespace xor_detail {

template<size_t N>
struct XorString {
    char data[N];
    
    constexpr XorString(const char(&str)[N], uint8_t key) : data{} {
        for (size_t i = 0; i < N; ++i) {
            data[i] = str[i] ^ key;
        }
    }
    
    const char* decrypt(uint8_t key) const {
        static thread_local char decrypted[N];
        for (size_t i = 0; i < N; ++i) {
            decrypted[i] = data[i] ^ key;
        }
        return decrypted;
    }
};

constexpr uint8_t GenerateKey(const char* file, int line) {
    return static_cast<uint8_t>(
        (file[0] ^ line) + 
        (file[1] ^ (line >> 8)) + 
        0x5A
    ) | 1;
}

} // namespace xor_detail

#define xorstr_(str) \
    []() { \
        constexpr auto key = ::xor_detail::GenerateKey(__FILE__, __LINE__); \
        static constexpr ::xor_detail::XorString<sizeof(str)> encrypted(str, key); \
        return encrypted.decrypt(key); \
    }()

#define _XS(str) xorstr_(str)

