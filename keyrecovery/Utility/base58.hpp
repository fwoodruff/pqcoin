//
//  base58.hpp
//  keyrecovery
//
//  Created by Frederick Benjamin Woodruff on 14/05/2022.
//

#ifndef base58_hpp
#define base58_hpp

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <vector>
#include <array>
#include <optional>

#include "falcon.h"

#include <algorithm>
#include <string>

#include <assert.h>
#include <string.h>
#include <optional>
#include <limits>

constexpr int SUM_SIZE = 4;

/* All alphanumeric characters except for "0", "I", "O", and "l" */
static const char* pszBase58 = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";
static const int8_t mapBase58[256] = {
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    -1, 0, 1, 2, 3, 4, 5, 6,  7, 8,-1,-1,-1,-1,-1,-1,
    -1, 9,10,11,12,13,14,15, 16,-1,17,18,19,20,21,-1,
    22,23,24,25,26,27,28,29, 30,31,32,-1,-1,-1,-1,-1,
    -1,33,34,35,36,37,38,39, 40,41,42,43,-1,44,45,46,
    47,48,49,50,51,52,53,54, 55,56,57,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
};





template<int S>
std::optional<std::array<uint8_t, S>> DecodeBase58(const std::string& str) {

    for(char c : str) if(c == '\0') return std::nullopt;

    const char* psz = str.data();
    
    // Skip leading spaces.
    while (*psz && std::isspace(*psz))
        psz++;
    // Skip and count leading '1's.
    int zeroes = 0;
    int length = 0;
    while (*psz == '1') {
        zeroes++;
        if (zeroes > S) return std::nullopt;
        psz++;
    }
    // Allocate enough space in big-endian base256 representation.
    ssize_t size = strlen(psz) * 733 /1000 + 1; // log(58) / log(256), rounded up. 1124/1535 is closer
    std::vector<unsigned char> b256(size);
    // Process the characters.
    static_assert(std::size(mapBase58) == 256, "mapBase58.size() should be 256"); // guarantee not out of range
    while (*psz && !std::isspace(*psz)) {
        // Decode base58 character
        int carry = mapBase58[(uint8_t)*psz];
        if (carry == -1)  // Invalid b58 character
            return std::nullopt;
        int i = 0;
        for (std::vector<unsigned char>::reverse_iterator it = b256.rbegin(); (carry != 0 || i < length) && (it != b256.rend()); ++it, ++i) {
            carry += 58 * (*it);
            *it = carry % 256;
            carry /= 256;
        }
        assert(carry == 0);
        length = i;
        if (length + zeroes > S) return std::nullopt;
        psz++;
    }
    // Skip trailing spaces.
    while (std::isspace(*psz))
        psz++;
    if (*psz != 0)
        return std::nullopt;
    
    
    
    
    // Skip leading zeroes in b256.
    auto it = b256.begin() + (size - length);
    
    
    std::array<uint8_t, S> vch {};

    std::copy(it, b256.end(), vch.begin());
    
    
    return {vch};
}

template<int S>
std::string EncodeBase58(std::array<uint8_t, S> input)
{
    size_t idx = 0;
    // Skip & count leading zeroes.
    int zeroes = 0;
    int length = 0;
    while (input.size() > idx && input[idx] == 0) {
        idx++;
        zeroes++;
    }
    // Allocate enough space in big-endian base58 representation.
    size_t size = (input.size()-idx) * 138 / 100 + 1; // log(256) / log(58), rounded up.
    std::vector<unsigned char> b58(size);
    // Process the bytes.
    while (input.size() > idx) {
        int carry = input[idx];
        int i = 0;
        // Apply "b58 = b58 * 256 + ch".
        for (std::vector<unsigned char>::reverse_iterator it = b58.rbegin(); (carry != 0 || i < length) && (it != b58.rend()); it++, i++) {
            carry += 256 * (*it);
            *it = carry % 58;
            carry /= 58;
        }

        assert(carry == 0);
        length = i;
        idx++;
    }
    // Skip leading zeroes in base58 result.
    std::vector<unsigned char>::iterator it = b58.begin() + (size - length);
    while (it != b58.end() && *it == 0)
        it++;
    // Translate the result into a string.
    std::string str;
    str.reserve(zeroes + (b58.end() - it));
    str.assign(zeroes, '1');
    while (it != b58.end())
        str += pszBase58[*(it++)];
    return str;
}



template<int S>
std::optional<std::array<uint8_t, S>> decode_base58_checked(const std::string& str) {
    
    
    auto x = DecodeBase58<S+SUM_SIZE>(str);
    if(x == std::nullopt) {
        return std::nullopt;
    }
    std::array<uint8_t, S+SUM_SIZE> outc;
    std::copy_n(x->begin(), S, outc.begin());
    
    shake256_context ctx;
    
    shake256_init_prng_from_seed(&ctx, outc.data(), S);
    shake256_extract(&ctx, outc.data()+S, SUM_SIZE);
    
    if(x != outc) {
        return std::nullopt;
    }
    
    std::array<uint8_t, S> out;
    std::copy_n(x->begin(), S, out.begin());
    return out;
}


template<size_t S>
std::string encode_base58_checked(std::array<uint8_t, S> input) {
    std::array<uint8_t, S + SUM_SIZE> input_checked;
    std::copy_n(input.begin(), S, input_checked.begin());
    
    shake256_context ctx; // overkill but not performance critical
    shake256_init_prng_from_seed(&ctx, input.data(), input.size());

    shake256_extract(&ctx, input_checked.data()+S, SUM_SIZE);
    return EncodeBase58<S+SUM_SIZE>(input_checked);
}


#endif /* base58_hpp */
