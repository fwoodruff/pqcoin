//
//  currency.hpp
//  keyrecovery
//
//  Created by Frederick Benjamin Woodruff on 05/09/2022.
//

#ifndef currency_hpp
#define currency_hpp

#include "mempool.hpp"
#include "quarantine.hpp"
#include "block_history.hpp"
#include "blockstack.hpp"
#include "block_tree.hpp"

#include <cstdio>
#include <string>



class persistent {
    std::vector<collision> semi_verified_blocks_seen;
    std::vector<block_header> chain;
    uint64_t trusted;
    // utxo
    // blocks
    
    // trusted = 0
    // we assume utxo is right and initialise stack
    // mempool we zero init
    // quarantine we zero init
    // blocktree we init from chain
    // history we init from blocks_seen
};

class local_state {
    bool live; // false until told
     
    uint64_t trusted;
    blockstack chain;
    mempool pool;
    quarantine quarantine;
    blocktree tree;
    history hist;
    
    std::vector<block> reorg();
    

    std::string block_directory;
    
    
    
    
    std::string reboot_info; // trusted
    
public:
    local_state(std::string directory);
    
    std::vector<block> consume(const block& block);
    void consume(const signed_transaction& pool_tx);
    
    
    
    uint64_t balance_for_key(public_key key) const;
    
    std::optional<signed_transaction> generate_tx(private_key sender,
                                                  const std::vector<std::pair<public_key, uint64_t>>& recipients,
                                                  uint64_t fee) const;
    
    local_state(persistent);
    
};







#endif /* currency_hpp */



