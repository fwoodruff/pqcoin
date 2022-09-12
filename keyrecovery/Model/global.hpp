//
//  types.hpp
//  keyrecovery
//
//  Created by Frederick Benjamin Woodruff on 13/05/2022.
//

#ifndef types_hpp
#define types_hpp

#include "falcon.h"
#include "blocking_queue.hpp"
#include "cached_file_io.hpp"
#include "bignum.hpp"

#include <cstdio>
#include <array>
#include <vector>
#include <string>
#include <optional>
#include <iostream>
#include <algorithm>


/*
 51% attacks must be recoverable but with unpicking.
 51% DDoS leads to arbitrarily long reorderings, may have to trust signatures if there is a long shutdown
 */


using ustring = std::basic_string<uint8_t>;

using mtime_point = std::chrono::time_point<std::chrono::system_clock>;
using namespace std::chrono_literals;
using namespace std::chrono;

constexpr uint64_t MAX_COINS = 1ull<<50;
constexpr size_t MAX_OUTPUTS = 100'000;

constexpr size_t BLOCK_SIZE = 40'000'000;
constexpr size_t COIN_SIZE = 100'000'000;
constexpr size_t INITIAL_REWARD = 50;
constexpr size_t HALVING_RATE = 210'000;

// cryptographic parameters
constexpr size_t PREIMAGE_SIZE = 32; // needs to be preimage resistant
constexpr size_t COLLISION_SIZE = COLLISION_SIZE_INTERNAL; // collision resistance


// pruning
constexpr size_t DIFFICULTY_HISTORY = 10; // base difficulty on this many blocks
constexpr size_t SIDE_CHAIN_DEPTH = 300; // reject side chains lower
constexpr size_t SIGNATURE_VIEW_DEPTH = 5; // when joining network, verify a few signed blocks before participating
constexpr auto BLOCK_TIME = 10min;
constexpr size_t MAX_POOL = 1'000'000;

// networking
constexpr size_t OUTBOUND_PEERS = 9;
constexpr size_t INBOUND_MIN_PEERS = 1000; // keep at least this many peers
constexpr size_t INBOUND_MAX_PEERS = 1020; // dump a few each block, to keep things live




extern const std::string ROOT_DIRECTORY;
extern const std::string DATA_DIRECTORY;
extern const std::string BLOCK_FOLDER;

using namespace std::chrono;


enum struct action {
    shutdown,
    start_mining,
    stop_mining,
};

template<size_t size> struct serialisable_byte_array;
struct serialisable_byte_vector;

using collision = serialisable_byte_array<COLLISION_SIZE>;
using preimage = serialisable_byte_array<PREIMAGE_SIZE>;
using public_key = collision;
using private_key = preimage;
using signature = serialisable_byte_vector;


template<typename T, std::enable_if_t<!std::is_integral_v<T>, bool> = true>
T deserialise(const uint8_t*& start, const uint8_t* const end);

struct serialisable {
    virtual void append_serial(std::vector<uint8_t>& output) const = 0;
    std::vector<uint8_t> serialise() const;
    collision secure_hash() const;
    virtual ~serialisable() = default;
};

template<size_t SIZE>
struct serialisable_byte_array : serialisable {
    void append_serial(std::vector<uint8_t>& output) const override {
        output.insert(output.end(), this->v.begin(), this->v.end());
    }
    
    std::array<uint8_t, SIZE> v{};
    
    serialisable_byte_array(const std::array<uint8_t, SIZE>& arg) { v = arg; }
    serialisable_byte_array() = default;
    
    friend bool operator>(serialisable_byte_array lhs, serialisable_byte_array rhs) { return lhs.v > rhs.v; }
    friend bool operator<(serialisable_byte_array lhs, serialisable_byte_array rhs) { return lhs.v < rhs.v; }
    friend bool operator>=(serialisable_byte_array lhs, serialisable_byte_array rhs) { return lhs.v >= rhs.v; }
    friend bool operator<=(serialisable_byte_array lhs, serialisable_byte_array rhs) { return lhs.v <= rhs.v; }
    friend bool operator==(serialisable_byte_array lhs, serialisable_byte_array rhs) { return lhs.v == rhs.v; }
    friend bool operator!=(serialisable_byte_array lhs, serialisable_byte_array rhs) { return lhs.v != rhs.v; }
};

template <size_t SIZE>
serialisable_byte_array<SIZE> deserialise_byte_array(const uint8_t*& start, const uint8_t* const end) {
    if(start == end or start + SIZE > end) throw std::logic_error("string too short");
    serialisable_byte_array<SIZE> out;
    std::copy_n(start, SIZE, out.v.begin());
    start += SIZE;
    return out;
}


struct serialisable_byte_vector : serialisable {
    std::vector<uint8_t> v {};
    void append_serial(std::vector<uint8_t>& output) const override;
    serialisable_byte_vector(std::vector<uint8_t> arg) { v = arg; }
    serialisable_byte_vector() = default;
};

template<>
serialisable_byte_vector deserialise(const uint8_t*& start, const uint8_t* const end);


/*
template<typename T>
inline void serialise_int(T in, std::vector<uint8_t>& output) {
    for(int i = 0; i < sizeof(T); i++) {
        output.push_back(in >> (i*8));
    }
}
 */
 

inline void serialise_int(uint64_t in, std::vector<uint8_t>& output) {
    assert( in < (1ull << 62));
    for(uint64_t i = 0; i < 4; i++) {
        uint64_t width = 1 << i;
        uint64_t maxint = 1ull << (width * 8 - 2);
        if(in < maxint) {
            in <<= 2;
            in |= i;
            for(int i = 0; i < width; i++) {
                output.push_back(in >> (i*8));
            }
            return;
        }
    }
}



inline uint64_t deserialise_intvar(const uint8_t*& start, const uint8_t* const end) {
    uint64_t result = 0;
    uint64_t width;
    if(start == end or start + 1 > end) goto END;
    width = (1 << ((*start) & 0b11));
    if(start + width > end) goto END;
    for(int i = 0; i < width; i++) {
        result <<= 8;
        result |= start[width - i - 1];
    }
    result >>= 2;
    start += width;
    return result;
END:
    throw std::logic_error("string too short");
}
 

/*
template<typename T, std::enable_if_t<std::is_integral_v<T>, bool> = true>
T deserialise(const uint8_t*& start, const uint8_t* end) {
    
    if(start == end or start + sizeof(T) > end) throw std::logic_error("string too00 short");
    T result = 0;
    for(int i = 0; i < sizeof(T); i++) {
        result <<= 4;
        result <<= 4;
        result |= start[sizeof(T) - i - 1];
    }
    start += sizeof(T);
    return result;
}
 */






template<typename T, std::enable_if_t<std::is_integral_v<T>, bool> = true>
T deserialise(const uint8_t*& start, const uint8_t* const end) {
    static_assert(!std::is_same_v<T, uint8_t>);
    return static_cast<T>(deserialise_intvar(start, end));
}


template<typename T>
void serialise_vec(const std::vector<T>& objects, std::vector<uint8_t>& output) {
    serialise_int(uint32_t(objects.size()), output);
    for(const auto& obj : objects) {
        obj.append_serial(output);
    }
}

template<>
inline time_point<system_clock> deserialise(const uint8_t*& start, const uint8_t* const end) {
    uint64_t count = deserialise<uint64_t>(start, end);
    return time_point<system_clock>() + milliseconds(count);
}

inline void serialise_timepoint(time_point<system_clock> tp,  std::vector<uint8_t>& output) {
    serialise_int(duration_cast<milliseconds>(tp.time_since_epoch()).count(), output);
}


template<typename U>
std::vector<U> deserialise_vec(const uint8_t*& start, const uint8_t* const end) {
    std::vector<U> result;
    uint32_t size = deserialise<uint32_t>(start, end);
    for(int i = 0; i < size; i ++) {
        auto val = deserialise<U>(start, end);
        result.push_back(val);
    }
    return result;
}

// I hate this
inline bool operator==(const serialisable& lhs, const serialisable& rhs) {
    return lhs.serialise() == rhs.serialise();
}

inline bool operator!=(const serialisable& lhs, const serialisable& rhs) {
    return !(lhs == rhs);
}

collision difficulty_target(milliseconds block_time, collision phash);




#endif /* types_hpp */
