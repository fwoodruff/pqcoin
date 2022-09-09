//
//  block_history.hpp
//  keyrecovery
//
//  Created by Frederick Benjamin Woodruff on 09/09/2022.
//

#ifndef block_history_hpp
#define block_history_hpp

#include <stdio.h>

#include "blocks.hpp"
#include <string>
#include <vector>
#include "utxo_index.hpp"

class history {
    std::unordered_map<collision, size_t> entry;
    std::string collision_history_file;
public:
    std::vector<collision> hist;
    void add_entry(collision);
    size_t fetch_index(collision);
    history(std::string directory);
};

#endif /* block_history_hpp */
