//
//  bloom.cpp
//  keyrecovery
//
//  Created by Frederick Benjamin Woodruff on 14/05/2022.
//

#include "bloom.hpp"
#include "global.hpp"
#include <string>
#include "blocks.hpp"
#include "transaction.hpp"

// list of pubkey debits should include mining fees


std::pair<uint64_t, uint64_t> bloom_filter::pair_hash(const public_key& key) {
    const uint8_t* v = &key.v[0];
    uint64_t val1 = deserialise<uint64_t>(v, key.v.end());
    uint64_t val2 = deserialise<uint64_t>(v, key.v.end());
    return {val1, val2};
}

void bloom_filter::insert(const public_key& key) {
    auto [val1, val2] = bloom_filter::pair_hash(key);
    v[val1 >> 3] |= 1 << ( val1 & 7 );
    v[val2 >> 3] |= 1 << ( val2 & 7 );
}

bool bloom_filter::might_contain(const public_key& key) const {
    auto [v1, v2] = bloom_filter::pair_hash(key);
    auto a = v[v1 >> 3] & (1 << ( v1 & 7 ));
    auto b = v[v2 >> 3] & (1 << ( v2 & 7 ));
    return (a and b);
}

bloom_filter::bloom_filter(size_t num_keys) {
    auto bits = num_keys * 11;
    auto bytes = bits/8 + 1;
    num_elems = num_keys;
    v.resize(bytes);
    
}


