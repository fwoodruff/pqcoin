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




class local_state {
    /*
     [[TODON'T]] optimisation: cache blocks
     */
    bool live;
    uint64_t trusted;
    blockstack chain;
    mempool pool;
    quarantine quarantine;
    blocktree tree;
    history hist;
    std::string block_directory;
    std::string reboot_info; // trusted
    
    std::vector<block> reorg();
public:
    local_state(std::string directory);
    
    
    std::vector<block> consume(const block& block);
    void consume(const signed_transaction& pool_tx);
    
    uint64_t balance_for_key(public_key key) const;
    std::optional<block> mine(public_key recipient) const;
    std::optional<signed_transaction> generate_tx(private_key sender,
                                                  const std::vector<std::pair<public_key, uint64_t>>& recipients,
                                                  uint64_t fee) const;
    
    
};







#endif /* currency_hpp */



