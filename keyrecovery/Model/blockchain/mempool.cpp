//
//  mempool.cpp
//  keyrecovery
//
//  Created by Frederick Benjamin Woodruff on 09/09/2022.
//

#include "mempool.hpp"

#include "merkle_tree.hpp"


/*
 After a new block, we need to creae a new block to mine
 */
block mempool::generate_unmined_block(public_key recipient, std::optional<block_header> latest) const {
    using namespace std::chrono;
    block b;

    b.txns = mempool;
    
    std::vector<collision> pool_hash;
    uint64_t f = 0;
    for(const auto& tx : b.txns) {
        pool_hash.push_back(tx.tx.secure_hash());
        f += tx.tx.fee;
    }
    
    b.h.timestamp = time_now();
    b.h.miner = recipient;
    
    
    if(!latest.has_value()) {
        assert(mempool.size() == 0);
        // root block
        b.h.previous_block_hash = {};
        b.h.merkle_root = merkle_tree({}).root();
        b.h.block_height = 0;
        b.h.mining_reward_and_fees = mining_reward(0);
        b.h.num_tx = 0;
        random_bytes(b.h.mining_nonce.v.data(), b.h.mining_nonce.v.size());
    } else {
        // block
        b.h.previous_block_hash = latest->secure_hash();
        b.h.merkle_root = merkle_tree(pool_hash).root();
        b.h.block_height = latest->block_height + 1;
        b.h.mining_reward_and_fees = f + mining_reward(b.h.block_height);
        b.h.num_tx = static_cast<uint32_t>(b.txns.size());
        random_bytes(b.h.mining_nonce.v.data(), b.h.mining_nonce.v.size());
    }
    return b;
}


void mempool::consume_new_block(const block& new_block) {
    std::unordered_set<tx_input> block_inputs;
    for(auto stx : new_block.txns) for(auto ipp : stx.tx.inputs) { block_inputs.insert(ipp); }
    
    mempool.erase(std::remove_if(mempool.begin(), mempool.end(), [&](const auto& item){
        return std::any_of(item.tx.inputs.begin(), item.tx.inputs.end(), [&](const auto& inp){
            return block_inputs.find(inp) != block_inputs.end();
        });
    }), mempool.end());
    
    for(const auto& inp : block_inputs) {
        mempool_inputs.erase(inp);
    }
}

void mempool::remove_block(const block& b) {
    [[TODO]];
    
    
}


bool mempool::consume_transaction(const signed_transaction& tx) {
    for(auto inp : tx.tx.inputs) {
        auto it = mempool_inputs.find(inp);
        if(it != mempool_inputs.end()) {
            return false;
        }
    }
    mempool.push_back(tx);
    for(auto inp : tx.tx.inputs) {
        mempool_inputs.insert(inp);
    }
    return true;
}

bool mempool::empty() const {
    assert( mempool.empty() == mempool_inputs.empty());
    return mempool.empty();
}

void mempool::clear() {
    mempool.clear();
    mempool_inputs.clear();
}
