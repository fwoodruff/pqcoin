//
//  blockchain.hpp
//  keyrecovery
//
//  Created by Frederick Benjamin Woodruff on 26/08/2022.
//

#ifndef blockchain_hpp
#define blockchain_hpp

#include <cstdio>
#include <string>
#include <vector>

#include "utxo_index.hpp"
#include "blocks.hpp"



class blockstack {
private:
    [[TODO]];
    /*
     mechanism for accessing old (7 days etc.) blocks and removing signatures
     if we put this in history, we allow 'easy' late timestamp blocks to block signature removal
     */
    tx_index index;
    block_header latest;

    std::string latest_file;
    std::string block_directory;
public:
    blockstack(std::string directory);
    blockstack() = default;
    blockstack& operator=(blockstack&& other) = default;
    blockstack(blockstack&&) = default;
    blockstack(const blockstack& ) = delete;
    blockstack& operator=(const blockstack& other) = delete;
    
    bool verify_inputs(const block& bl) const;
    bool verify_spend_mempool(const signed_transaction& tx) const;
    uint64_t balance_for_key(public_key key) const;
    std::optional<signed_transaction> generate_tx(private_key sender,
                                                  const std::vector<std::pair<public_key, uint64_t>>& recipients,
                                                  uint64_t fee) const;
    
    bool fits_on_top(const block& new_block) const;

    
    void push_block(const block& bl);
    block pop_block();
    block_header top_header() const;

    
};

#endif /* blockchain_hpp */
