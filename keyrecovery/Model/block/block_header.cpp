//
//  block_header.cpp
//  keyrecovery
//
//  Created by Frederick Benjamin Woodruff on 09/09/2022.
//
#include "global.hpp"
#include "block_header.hpp"



bool block_header::verify_header(uint64_t previous_time, const block_header& latest) const {
    if(block_height > latest.block_height + SIDE_CHAIN_DEPTH) { return false; }
    if(block_height + SIDE_CHAIN_DEPTH > latest.block_height) { return false; }
    assert(previous_block_hash == latest.secure_hash());
    if(verify_difficulty(previous_time)) { return false; }
    if(timestamp <= latest.timestamp ) { return false; }
    return true;
}

bool block_header::verify_time() const {
    using namespace std::chrono;
    uint64_t now_ms = time_now();
    return now_ms >= timestamp;
}


bool block_header::verify_difficulty(uint64_t prev_block_time) const {
    collision target = difficulty_target(prev_block_time, previous_block_hash);
    collision actual = secure_hash();
    return actual < target;
}

bool mining_attempt(block_header& header, uint64_t prev_block_time, uint64_t attempts) {
    assert(attempts > 100);
    static_assert(PREIMAGE_SIZE % sizeof(uint64_t) == 0, "bad nonce size");
    
    random_bytes(header.mining_nonce.v.data(), header.mining_nonce.v.size());
    header.timestamp = time_now();
    
    auto blkserial = header.serialise();
    
    collision target = difficulty_target(prev_block_time, header.previous_block_hash);
    
    for(int i = 0; i < attempts; i++) {
        memcpy(blkserial.data(), &i, sizeof(uint64_t));
        shake256_context ctx;
        collision res;
        shake256_init_prng_from_seed(&ctx, blkserial.data(), blkserial.size());
        shake256_extract(&ctx, res.v.data(), res.v.size());
        
        if(res < target) {
            const uint8_t * b = &*blkserial.begin();
            const uint8_t * e = &*blkserial.end();
            
            header = deserialise<block_header>(b, e);
            return true;
        }
    }
    return false;
}



void block_header::append_serial(std::vector<uint8_t>& output) const {
    mining_nonce.append_serial(output);
    previous_block_hash.append_serial(output);
    merkle_root.append_serial(output);
    miner.append_serial(output);
    serialise_int(block_height, output);
    serialise_int(timestamp , output);
    serialise_int(num_tx , output);
    serialise_int(mining_reward_and_fees, output);
}

template<>
block_header deserialise(const uint8_t*& start, const uint8_t* const end) {
    block_header out;
    out.mining_nonce = deserialise<preimage>(start, end);
    out.previous_block_hash = deserialise<collision>(start, end);
    out.merkle_root = deserialise<collision>(start, end);
    out.miner = deserialise<public_key>(start, end);
    out.block_height = deserialise<uint64_t>( start, end);
    out.timestamp = deserialise<uint64_t>(start, end);
    out.num_tx = deserialise<uint32_t>(start, end);
    out.mining_reward_and_fees = deserialise<uint64_t>(start, end);

    return out;
}
