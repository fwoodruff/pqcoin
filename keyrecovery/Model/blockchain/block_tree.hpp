//
//  block_tree.hpp
//  keyrecovery
//
//  Created by Frederick Benjamin Woodruff on 09/09/2022.
//

#ifndef block_tree_hpp
#define block_tree_hpp


#include "utxo_index.hpp"
#include "blocks.hpp"

#include <cstdio>
#include <string>
#include <vector>

/*
 stores a recent tree of blocks to verify that new blocks are connected and new
 */
class blocktree {
    std::string block_directory;
    std::string recent_header_file;
    std::unordered_map<collision, block_header> hash_header;
public:
    blocktree(std::string rootdirectory);
    bool verify_block(const block& block) const;
    void consume_block(const block& block);

    std::optional<collision> fork_point(collision lhs, collision rhs) const;
};


#endif /* block_tree_hpp */
