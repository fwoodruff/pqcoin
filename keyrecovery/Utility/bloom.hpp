//
//  bloom.hpp
//  keyrecovery
//
//  Created by Frederick Benjamin Woodruff on 14/05/2022.
//

#ifndef bloom_hpp
#define bloom_hpp

#include <stdio.h>
#include <vector>
#include <utility>

#include "global.hpp"


class bloom_filter {
    size_t num_elems;
    std::vector<uint8_t> v;
    static std::pair<uint64_t, uint64_t> pair_hash(const public_key& key);
    
public:
    void insert(const public_key& key);
    bloom_filter(size_t num_keys);
    bool might_contain(const public_key& key) const;
    friend class net_block;
};


#endif /* bloom_hpp */
