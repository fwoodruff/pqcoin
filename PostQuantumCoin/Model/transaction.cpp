//
//  transaction.cpp
//  keyrecovery
//
//  Created by Frederick Benjamin Woodruff on 13/05/2022.
//


#include "global.hpp"
#include "transaction.hpp"
#include "signatures.hpp"

#include "blocks.hpp"
#include <vector>
#include <array>
#include <optional>
#include <random>
#include <map>
#include <set>
#include <memory>
#include <unordered_set>

// tx_output
void tx_output::append_serial(std::vector<uint8_t>& output) const {
    serialise_int(amount, output);
    recipient.append_serial(output);
}
template<>
tx_output deserialise(const uint8_t*& start, const uint8_t* end) {
    tx_output out;
    out.amount = deserialise<uint64_t>(start, end);
    out.recipient = deserialise<public_key>(start, end);
    return out;
}

// tx_input
void tx_input::append_serial(std::vector<uint8_t>& output) const {
    previous_tx.append_serial(output);
}
template<>
tx_input deserialise(const uint8_t*& start, const uint8_t* end) {
    tx_input in;
    in.previous_tx = deserialise<collision>(start, end);
    return in;
}

// transaction
void transaction::append_serial(std::vector<uint8_t>& out) const {
    key.append_serial(out);
    serialise_int(fee, out);
    serialise_vec(inputs, out);
    serialise_vec(outputs, out);
}
template<>
transaction deserialise(const uint8_t*& start, const uint8_t* end) {
    transaction tx {};
    tx.key = deserialise<public_key>(start, end);
    tx.fee = deserialise<uint64_t>(start, end);
    tx.inputs = deserialise_vec<tx_input>(start, end);
    tx.outputs = deserialise_vec<tx_output>(start, end);
    return tx;
}

// signed_transaction
void signed_transaction::append_serial(std::vector<uint8_t>& out) const {
    tx.append_serial(out);
    sig.append_serial(out);
}
template<>
signed_transaction deserialise(const uint8_t*& start, const uint8_t* end) {
    signed_transaction tx {};
    tx.tx = deserialise<transaction>(start, end);
    tx.sig = deserialise<signature>(start, end);
    return tx;
}


bool transaction::verify() const {
    std::unordered_set<public_key> pp;
    
    for(const auto& output : outputs) {
        auto [v,q] = pp.insert(output.recipient);
        if(!q) { return false; }
        
        if(output.amount > MAX_COINS) {
            return false;
        }
    }
    for(const auto& input : inputs) {
        auto [v,q] = pp.insert(input.previous_tx);
        if(!q) { return false; }
    }
    return true;
}


bool signed_transaction::verify_sig() const {
    tx.verify();
    std::vector<uint8_t> o;
    tx.append_serial(o);
    return verify_signature(sig, tx.key, o);
}




