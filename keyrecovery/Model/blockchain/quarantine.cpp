//
//  quarantine.cpp
//  keyrecovery
//
//  Created by Frederick Benjamin Woodruff on 09/09/2022.
//

#include "quarantine.hpp"



void quarantine::evict_shallow(uint64_t current_height) {
    v.erase(std::remove_if(v.begin(), v.end(), [&](const auto& item){
        return item.h.block_height + SIDE_CHAIN_DEPTH > current_height;
    }), v.end() );
}


void quarantine::evict_block(const block& block) {
    v.erase(std::remove(v.begin(), v.end(), block), v.end());
}
