//
//  global.cpp
//  keyrecovery
//
//  Created by Frederick Benjamin Woodruff on 28/05/2022.
//

#include "global.hpp"

#include "blocking_queue.hpp"

#include <cstdio>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>


const std::string ROOT_DIRECTORY = "/Users/freddiewoodruff/Documents/Programming/keyrecovery/keyrecovery/";
const std::string DATA_DIRECTORY = ROOT_DIRECTORY + "Data/";

std::vector<uint8_t> serialisable::serialise() const {
    std::vector<uint8_t> output;
    this->append_serial(output);
    return output;
}

collision serialisable::secure_hash() const {
    auto bytes = serialise();
    shake256_context ctx;
    shake256_init(&ctx);
    uint8_t prefix = 1;
    shake256_inject(&ctx, &prefix, 1);
    shake256_inject(&ctx, bytes.data(), bytes.size());
    shake256_flip(&ctx);
    collision result;
    shake256_extract(&ctx, result.v.data(), result.v.size());
    return result;
}

void serialisable_byte_vector::append_serial(std::vector<uint8_t>& output) const {
    serialise_int(uint32_t(this->v.size()), output);
    output.insert(output.end(), this->v.begin(), this->v.end());
}

template<>
serialisable_byte_vector deserialise(const uint8_t*& start, const uint8_t* end) {
    auto size = deserialise<uint32_t>(start, end);
    if(size > BLOCK_SIZE) {
        throw std::logic_error("bad object size");
    }
    serialisable_byte_vector result;
    result.v.resize(size);
    
    std::copy_n(start, size, result.v.begin());
    start += size;
    
    return result;
}


// function partial template specialisation not allowed
template<>
serialisable_byte_array<PREIMAGE_SIZE> deserialise(const uint8_t*& start, const uint8_t* end) {
    return deserialise_byte_array<PREIMAGE_SIZE>(start, end);
}

template<>
serialisable_byte_array<COLLISION_SIZE> deserialise(const uint8_t*& start, const uint8_t* end) {
    return  deserialise_byte_array<COLLISION_SIZE>(start, end);
}





collision difficulty_target(uint64_t time_dif, collision phash) {
    using bigint = fbw::uVar<COLLISION_SIZE*CHAR_BIT>;
    
    std::reverse(phash.v.begin(), phash.v.end());
    const auto previous_small_value = bigint(phash.v);


    const auto achievement_time = bigint(time_dif);
    constexpr auto target_time = bigint(BLOCK_TIME_MS);
    
    constexpr auto ONE = bigint(1);
    constexpr auto BIGGEST_SMALL_NUMBER = (ONE*ONE) << (COLLISION_SIZE*CHAR_BIT - 4);
    
    auto new_small_value = (previous_small_value * achievement_time)/ target_time;
    if(new_small_value > BIGGEST_SMALL_NUMBER) { new_small_value = BIGGEST_SMALL_NUMBER; }
    
    auto small_no = bigint(new_small_value);
    auto small_serial = small_no.serialise();
    
    collision res;
    res.v = small_serial;
    std::reverse(res.v.begin(), res.v.end());
    
    return res;
}


