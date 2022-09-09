//
//  blocks.cpp
//  keyrecovery
//
//  Created by Frederick Benjamin Woodruff on 13/05/2022.
//



#include "blocks.hpp"
#include "global.hpp"
#include "falcon.h"
#include "signatures.hpp"
#include "transaction.hpp"
#include "base58.hpp"
#include "cached_file_io.hpp"
#include "merkle_tree.hpp"

#include <vector>
#include <array>
#include <fstream>
#include <sstream>
#include <unordered_set>




/* ******************* SERIALISATION ***************** */



// block
void block::append_serial(std::vector<uint8_t>& output) const {
    h.append_serial(output);
    serialise_vec<signed_transaction>(txns, output);
    // don't need to serialise lots of null signatures
}
template<>
block deserialise(const uint8_t*& start, const uint8_t* const end) {
    block block;
    block.h = deserialise<block_header>(start, end);
    block.txns = deserialise_vec<signed_transaction>(start, end);
    return block;
}


/* ********************** FUNCTIONS ************************* */




void block::remove_signatures() {
    for(auto& t : txns) {
        t.sig.v.clear();
    }
}


uint64_t mining_reward(uint64_t height) {
    uint64_t hv = height/HALVING_RATE;
    if(hv > (sizeof(uint64_t) * CHAR_BIT - 1) ) {
        return 0;
    }
    return (COIN_SIZE * INITIAL_REWARD) >> hv;
}



void write_block(std::string directory, block bloc) {
    auto bytes = bloc.serialise();
    auto new_block_hash = bloc.h.secure_hash();
    std::string new_block_name = directory + encode_base58_checked(new_block_hash.v) + std::string(".block");
    write_to_file(new_block_name, bytes);
}

block read_block(std::string directory, collision blockhash) {
    std::string filename = directory + encode_base58_checked(blockhash.v) + std::string(".block");
    auto bytes = read_file(filename);
    const uint8_t* bytes_start = bytes.data();
    const uint8_t* bytes_end = bytes.data() + bytes.size();
    return deserialise<block>(bytes_start, bytes_end);
};







/* **************** VERIFICATION **************** */




bool block::verify_signatures() const {
    return std::all_of(txns.begin(), txns.end(), [](const auto& tx){ return tx.verify_sig() ;});
}

bool block::verify_merkle_root() const {
    std::vector<collision> leaves;
    for(const auto& tx : txns) {
        leaves.push_back(tx.secure_hash());
    }
    auto candidate_root = merkle_tree(leaves).root();
    return candidate_root == h.merkle_root;
}

bool block::verify_reward() const {
    uint64_t fees = 0;
    for(auto tx : this->txns ) {
        fees += tx.tx.fee;
    }
    auto reward = mining_reward(h.block_height);
    return (fees + reward == h.mining_reward_and_fees);
}

bool block::verify_unique_inputs() const {
    std::unordered_set<tx_input> block_inputs;
    for(auto stx : txns) for(auto ipp : stx.tx.inputs) {
        auto [it, succ] = block_inputs.insert(ipp);
        if(!succ) { return false; }
    }
    return true;
}


bool block::verify_unsigned(const block_header& latest, uint64_t previous_time) const {
    h.verify_header(previous_time, latest);
    if(h.num_tx != txns.size()) { return false; }
    if(!verify_merkle_root()) { return false; }
    if(!verify_reward()) { return false; }
    if(!verify_unique_inputs()) { return false; }
    return true;
}


bool block::verify_root() const {
    if(h.previous_block_hash != collision{}) { return false; };
    if(h.merkle_root != merkle_tree({}).root()) { return false;; };
    if(h.block_height != 0) { return false; }
    if(h.mining_reward_and_fees != mining_reward(0)) { return false; }
    if(h.num_tx != 0) { return false; }
    return true;
}


bool block::has_signatures() const {
    if(txns.size() == 0) {
        return true;
    }
    return !txns[0].sig.v.empty(); // need to check all or serialise differently
}

