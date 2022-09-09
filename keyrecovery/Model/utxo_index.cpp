//
//  utxo_index.cpp
//  keyrecovery
//
//  Created by Frederick Benjamin Woodruff on 28/08/2022.
//

#include "utxo_index.hpp"
#include "base58.hpp"

#include <string>



tx_index::tx_index(std::string txhash_dir, std::string pubkey_dir) : tx_index() {
    make_directory(txhash_dir);
    make_directory(pubkey_dir);
}

static const std::string utxo_extension = ".utxo";
static const std::string pub_extension = ".pub";

std::vector<tx_output> tx_index::file_system_fetch_utxo(tx_input txhash) const {
    std::string txhash_filename = txhash_directory + EncodeBase58<48>(txhash.previous_tx.v) + utxo_extension;
    auto data = read_file(txhash_filename);

    if(data.empty()) {
        return {};
    }
    const uint8_t* start = data.data();
    const uint8_t* end = data.data() + data.size();
    try {
        auto out = deserialise_vec<tx_output>(start, end);
        assert(start == end);
        return out;
    } catch (...) {
        assert(false);
    }
    return {};
}

std::vector<utxo_ref> tx_index::file_system_fetch_utxo(public_key key) const {
    std::string txhash_filename = txhash_directory + EncodeBase58<48>(key.v) + pub_extension;
    auto data = read_file(txhash_filename);
    if(data.empty()) {
        return {};
    }
    const uint8_t* start = data.data();
    const uint8_t* end = data.data() + data.size();
    
    try {
        auto out = deserialise_vec<utxo_ref>(start, end);
        assert(start == end);
        return out;
    } catch (...) {
        assert(false);
    }
    return {};
}


void tx_index::file_system_overwrite_utxo(tx_input obj, const std::vector<tx_output>& new_outputs) {
    std::string filename = txhash_directory + EncodeBase58<48>(obj.previous_tx.v) + utxo_extension;
    
    if(new_outputs.empty()) {
        remove_file(filename);
        return;
    }
    std::vector<uint8_t> write_out;
    serialise_vec(new_outputs, write_out); // make functional version
    const uint8_t* start = write_out.data();
    const uint8_t* end = write_out.data() + write_out.size();
    auto res = deserialise_vec<tx_output>(start, end);

    write_to_file(filename, write_out);
}


void tx_index::file_system_overwrite_utxo(public_key key, const std::vector<utxo_ref>& out) {
    std::string filename = pubkey_directory + EncodeBase58<48>(key.v) + pub_extension;
    
    if(out.empty()) {
        remove_file(filename);
        return;
    }
    
    std::vector<uint8_t> write_out;
    serialise_vec(out, write_out);
    write_to_file(filename, write_out);
}

/*
 
 a new transaction input points at map-outputs that can no longer be spent
 lookup that (output) from input (txhash), and remove
 lookup that (output,txhash) from pubkey, and remove
 
 a new transaction output becomes a map-output that can now be spent
 lookup that (output) from input (txhash), and add
 lookup that (output,txhash), and add

 
 */


void tx_index::remove_utxo(tx_input txhash, public_key remkey) {

    auto outs = file_system_fetch_utxo(txhash);

    for(auto it = outs.begin() ; ; it++) {
        assert(it != outs.end());
        if(it->recipient == remkey) {
            outs.erase(it);
            break;
        }
    }
    auto pubs = file_system_fetch_utxo(remkey);
    
    for(auto it = pubs.begin(); it != pubs.end(); it++) {
        if(it->out.recipient == remkey) {
            pubs.erase(it);
            break;
        }
    }
    file_system_overwrite_utxo(txhash, outs);
    file_system_overwrite_utxo(remkey, pubs);
}

void tx_index::add_utxo(tx_input txhash, std::vector<tx_output> out) {

    
    
    
    auto txhouts = file_system_fetch_utxo(txhash);
    
    assert(txhouts.empty());
    for(const auto& txo : out) {
        txhouts.push_back(txo);
    }
    file_system_overwrite_utxo(txhash, txhouts);
    auto test = file_system_fetch_utxo(txhash);
    assert(!txhouts.empty());
    for(const auto& op : out) {
        auto utxos = file_system_fetch_utxo(op.recipient);
        utxo_ref ut;
        ut.out = op;
        ut.txhash = txhash;
        utxos.push_back(ut);
        file_system_overwrite_utxo(op.recipient, utxos);
    }
}

void tx_index::add_block(const block& bl) {

    
    for(auto tx : bl.txns) {
        for(auto inpu : tx.tx.inputs) {
            remove_utxo(inpu, tx.tx.key);
        }
        tx_input inp;
        inp.previous_tx = tx.secure_hash();
        auto va = file_system_fetch_utxo(inp);
        assert(va.empty());
        add_utxo(inp, tx.tx.outputs);
    }
    tx_output miner;
    miner.recipient = bl.h.miner;
    
    miner.amount = bl.h.mining_reward_and_fees;
    tx_input block_input;
    block_input.previous_tx = bl.h.secure_hash();
    add_utxo(block_input, {miner});

    
}

void tx_index::remove_block(const block& current, const block& previous) {
    std::unordered_map<tx_input, transaction> tx_unhash;
    for(const auto& tx : previous.txns) {
        tx_input ip;
        ip.previous_tx = tx.secure_hash();
    }
    for(const auto& tx : current.txns) {
        tx_input ip;
        ip.previous_tx = tx.secure_hash();
        for(const auto& output : tx.tx.outputs) {
            remove_utxo(ip, output.recipient);
        }
        for(const auto& input : tx.tx.inputs) {
            auto tx_it = tx_unhash.find(input);
            assert(tx_it != tx_unhash.end());
            const auto& old_tx = tx_it->second;
            auto op_it = std::find(old_tx.outputs.begin(), old_tx.outputs.end(), tx.tx.key);
            assert(op_it != old_tx.outputs.end());
            add_utxo(input, {*op_it});
        }
    }
    // remove fee utxo
}
    
bool tx_index::verify_spend(transaction tx) const {
    uint64_t amount_in = 0;
    bool res = tx.verify();
    if(!res) {
        return false;
    }
    for(auto in : tx.inputs) {
        auto outputs = file_system_fetch_utxo(in);

        int matches = 0;
        for(auto output : outputs) {
            if(output.recipient == tx.key) {
                matches++;
                assert(output.amount < MAX_COINS);
                amount_in += output.amount;
            }
        }
        if(matches == 0) {
            return false;
        }
        assert(matches <= 1);
    }
    uint64_t amount_out = tx.fee;
    for(auto out : tx.outputs) {
        assert(out.amount < MAX_COINS);
        amount_out += out.amount ;
    }
    if(amount_in != amount_out) {
        return false;
    }
    return true;
}

void utxo_ref::append_serial(std::vector<uint8_t>& output) const {
    out.append_serial(output);
    txhash.append_serial(output);
}

template<>
utxo_ref deserialise(const uint8_t*& start, const uint8_t* end) {
    utxo_ref out;
    out.out = deserialise<tx_output>(start, end);
    out.txhash = deserialise<tx_input>(start, end);
    return out;
}
