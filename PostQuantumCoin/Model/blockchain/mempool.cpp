//
//  mempool.cpp
//  keyrecovery
//
//  Created by Frederick Benjamin Woodruff on 09/09/2022.
//

#include "mempool.hpp"

#include "merkle_tree.hpp"



void mempool::sort_pool() {
    std::sort(pool.begin(), pool.end(), [](const auto& lhs, const auto& rhs){
        return lhs.tx.fee * rhs.tx.outputs.size() < rhs.tx.fee * lhs.tx.outputs.size();
    });
    
    // give a few low fee transactions a chance
    uint64_t rnd = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    for(int i = 0; i < 20; i++) {
        uint64_t rnd2 = rnd * 33 + 5;
        std::swap(pool[rnd % pool.size()], pool[rnd2 % pool.size()]);
        rnd = rnd2;
    }
    
}

block mempool::generate_unmined_block(public_key recipient, block_header latest) const {
    block b;
    size_t blocksize = b.h.serialise().size() + 8;
    for(const auto& tx : pool) {
        auto stx = tx.serialise();
        if(blocksize + stx.size() > BLOCK_SIZE) {
            break;
        }
        blocksize += stx.size();
        b.txns.push_back(tx);
    }
    
    std::vector<collision> pool_hash;
    uint64_t f = 0;
    for(const auto& tx : b.txns) {
        pool_hash.push_back(tx.tx.secure_hash());
        f += tx.tx.fee;
    }
    
    b.h.timestamp = system_clock::now();
    b.h.miner = recipient;
    b.h.previous_block_hash = latest.secure_hash();
    b.h.merkle_root = merkle_tree(pool_hash).root();
    b.h.block_height = latest.block_height + 1;
    b.h.mining_reward_and_fees = f + mining_reward(b.h.block_height);
    b.h.num_tx = static_cast<uint32_t>(b.txns.size());
    random_bytes(b.h.mining_nonce.v.data(), b.h.mining_nonce.v.size());
    
    assert(b.serialise().size() < BLOCK_SIZE);
    
    return b;
}


void mempool::consume_new_block(const block& new_block) {
    std::unordered_set<tx_input> block_inputs;
    for(auto stx : new_block.txns) for(auto ipp : stx.tx.inputs) { block_inputs.insert(ipp); }
    
    pool.erase(std::remove_if(pool.begin(), pool.end(), [&](const auto& item){
        return std::any_of(item.tx.inputs.begin(), item.tx.inputs.end(), [&](const auto& inp){
            return block_inputs.find(inp) != block_inputs.end();
        });
    }), pool.end());
    
    for(const auto& inp : block_inputs) {
        mempool_inputs.erase(inp);
    }
    
}

void mempool::remove_block(const block& b) {
    assert(b.has_signatures());
    
    pool.insert(pool.end(), b.txns.begin(), b.txns.end());
    if(pool.size() > MAX_POOL) {
        pool.resize(MAX_POOL);
    }

}


bool mempool::consume_transaction(const signed_transaction& tx) {
    if(pool.size() > MAX_POOL) { return false; }
    for(auto inp : tx.tx.inputs) {
        auto it = mempool_inputs.find(inp);
        if(it != mempool_inputs.end()) {
            return false;
        }
    }
    pool.push_back(tx);
    for(auto inp : tx.tx.inputs) {
        mempool_inputs.insert(inp);
    }
    return true;
}

bool mempool::empty() const {
    assert( pool.empty() == mempool_inputs.empty());
    return pool.empty();
}

void mempool::clear() {
    pool.clear();
    mempool_inputs.clear();
}
