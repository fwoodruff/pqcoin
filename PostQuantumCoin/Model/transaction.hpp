//
//  transaction.hpp
//  keyrecovery
//
//  Created by Frederick Benjamin Woodruff on 13/05/2022.
//

#ifndef transaction_hpp
#define transaction_hpp


#include "signatures.hpp"
#include "global.hpp"

#include <vector>
#include <array>
#include <optional>
#include <random>
#include <map>
#include <set>
#include <unordered_map>

// tx_output
struct tx_output : serialisable {
    public_key recipient;
    uint64_t amount;
    void append_serial(std::vector<uint8_t>& output) const override;
};
template<>
tx_output deserialise(const uint8_t*& start, const uint8_t* const end);

template <> struct std::hash<tx_output> {
    inline size_t operator()(const tx_output & data) const {
        size_t hash = std::hash<collision>{}(data.recipient);
        hash = hash * 33 + data.amount;
        return hash;
    }
};


// tx_input
struct tx_input : serialisable {
    collision previous_tx;
    void append_serial(std::vector<uint8_t>& output) const override;
};
template<>
tx_input deserialise(const uint8_t*& start, const uint8_t* const end);

template <> struct std::hash<tx_input> {
    inline size_t operator()(const tx_input & data) const {
        return std::hash<collision>{}(data.previous_tx);
    }
};


// transaction
struct transaction : public serialisable {
    public_key key;
    uint64_t fee;
    std::vector<tx_input> inputs {};
    std::vector<tx_output> outputs {};
    void append_serial(std::vector<uint8_t>& output) const override;
    
    bool verify() const;
};
template<>
transaction deserialise(const uint8_t*& start, const uint8_t* const end);

// signed_transaction
struct signed_transaction : serialisable {
    transaction tx;
    signature sig;
    void append_serial(std::vector<uint8_t>& output) const override;
    bool verify_sig() const;
    
};

template<>
signed_transaction deserialise(const uint8_t*& start, const uint8_t* const end);








#endif /* transaction_hpp */
