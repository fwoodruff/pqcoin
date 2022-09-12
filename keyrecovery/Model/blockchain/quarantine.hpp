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
    [[TODO]];
    /*
     Quarantine needs to store block_headers in memory, and reach into block folder for blocks
     Blocks can blow memory, speed is not a concern.
     */
public:
    std::vector<block> v;
    void evict_shallow(uint64_t current_height);
    void evict_block(const block& block);
};


#endif /* quarantine_hpp */
