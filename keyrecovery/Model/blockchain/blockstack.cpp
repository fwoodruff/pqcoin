//
//  blockchain.cpp
//  keyrecovery
//
//  Created by Frederick Benjamin Woodruff on 26/08/2022.
//

#include "blockstack.hpp"
#include "merkle_tree.hpp"
#include "base58.hpp"
#include <string>
#include <chrono>
#include <fstream>
#include <algorithm>
#include <iostream>
#include <iterator>
#include <vector>
#include "global.hpp"






blockstack::blockstack(std::string rootdirectory) {
    make_directory(rootdirectory);
    index = tx_index ( rootdirectory + "utxos/", rootdirectory + "keys/" );
    latest_file = rootdirectory + "header.bin";
    
    auto data = read_file(latest_file);
    if(!data.empty()) {
        const uint8_t* b = &*data.begin();
        const uint8_t* e = &*data.end();
        latest = deserialise<block_header>(b, e);
    }
    
}


bool blockstack::verify_spend_mempool(const signed_transaction& tx) const {
    return tx.verify_sig() and index.verify_spend(tx.tx);
}



bool blockstack::fits_on_top(const block& new_block) const {
    if(!latest.has_value()) {
        return true;
    }
    return latest->secure_hash() == new_block.h.previous_block_hash;

}

std::optional<block> blockstack::top_block() const {
    [[TODO]];
    return {};
}


bool blockstack::verify_inputs(const block& new_block) const {

    return std::all_of(new_block.txns.begin(), new_block.txns.end(),
                       [&](const auto& x) {
        return index.verify_spend(x.tx);
    });
}

/*
 removes a block from the chain, as if it never existed.
 */
block blockstack::pop_block() {
    [[TODO]];
    return {};
    // should never pop a block with no signatures, throw
}





void blockstack::push_block(const block& new_block) {
    assert(fits_on_top(new_block));
    index.add_block(new_block);
    latest = new_block.h;
}

/*
 A user queries a balance
 */
uint64_t blockstack::balance_for_key(public_key key) const {
    auto utxo_refs = index.file_system_fetch_utxo(key);
    uint64_t result = 0;
    for(const auto& utxo_ref: utxo_refs) {
        result += utxo_ref.out.amount;
    }
    return result;
}

/*
 A user sends some coins
 */
std::optional<signed_transaction> blockstack::generate_tx(
    private_key sender,
    const std::vector<std::pair<public_key, uint64_t>>& recipients,
    uint64_t fee) const
{
    auto pubkey = make_pubkey(sender);
    auto utxo_refs = index.file_system_fetch_utxo(pubkey);
    
    std::vector<tx_input> inputs {};
    uint64_t coins = 0;
    for(const auto& utxo_ref: utxo_refs) {
        coins += utxo_ref.out.amount;
        inputs.push_back(utxo_ref.txhash);
    }
    std::vector<tx_output> outputs{};

    uint64_t spend = fee;
    for(const auto& [recipient, amount] : recipients) {
        spend += amount;
        tx_output output;
        output.recipient = recipient;
        output.amount = amount;
        outputs.push_back(output);
    }
    if(spend > coins) {
        return std::nullopt;
    }
    
    tx_output loopback;
    loopback.recipient = pubkey;
    loopback.amount = coins - spend;
    outputs.push_back(loopback);
    
    transaction tx;
    tx.key = pubkey;
    tx.outputs = outputs;
    tx.fee = fee;
    tx.inputs = inputs;

    signed_transaction sig_tx;

    sig_tx.tx = tx;
    sig_tx.sig = sign_message(sender, get_crsn(), tx.serialise());

    return sig_tx;
};









