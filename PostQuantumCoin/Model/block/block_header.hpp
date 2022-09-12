//
//  block_header.hpp
//  keyrecovery
//
//  Created by Frederick Benjamin Woodruff on 09/09/2022.
//

#ifndef block_header_hpp
#define block_header_hpp


#include "global.hpp"
#include "signatures.hpp"

#include <cstdio>

uint64_t mining_reward(uint64_t height);


struct block_header : serialisable {
    
    preimage mining_nonce;
    collision previous_block_hash;
    collision merkle_root;
    public_key miner;
    uint64_t block_height;
    time_point<system_clock> timestamp;
    uint64_t num_tx;
    uint64_t mining_reward_and_fees;
    
    bool verify_difficulty(milliseconds block_time) const;
    bool verify_header(milliseconds block_time, const block_header& latest) const ;
    bool verify_time() const;

    void append_serial(std::vector<uint8_t>& output) const override;
};


bool mining_attempt(block_header& header, milliseconds prev_block_duration, uint64_t attempts = 10000);

template<>
block_header deserialise(const uint8_t*& start, const uint8_t* const end);

#endif /* block_header_hpp */
