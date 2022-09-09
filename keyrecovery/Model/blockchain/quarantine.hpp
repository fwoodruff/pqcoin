//
//  quarantine.hpp
//  keyrecovery
//
//  Created by Frederick Benjamin Woodruff on 09/09/2022.
//

#ifndef quarantine_hpp
#define quarantine_hpp

#include <stdio.h>
#include "blocks.hpp"
#include <string>
#include <vector>
#include "utxo_index.hpp"


class quarantine {
public:
    std::vector<block> v;
    void evict_shallow(uint64_t current_height);
    void evict_block(const block& block);
};


#endif /* quarantine_hpp */
