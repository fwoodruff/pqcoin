//
//  mempool.hpp
//  keyrecovery
//
//  Created by Frederick Benjamin Woodruff on 09/09/2022.
//

#ifndef mempool_hpp
#define mempool_hpp


#include <stdio.h>
#include "blocks.hpp"
#include <string>
#include <vector>
#include "utxo_index.hpp"



class mempool {
    std::vector<signed_transaction> mempool ;
    std::unordered_set<tx_input> mempool_inputs;
public:
    void consume_new_block(const block& b); // removes those transactions from mempool
    void remove_block(const block& b); // remove transactions from b's pubkey
    bool consume_transaction(const signed_transaction& tx);
    block generate_unmined_block(public_key fee_recipient, std::optional<block_header> top ) const;
    [[nodiscard]] bool empty() const;
    void clear();
};




#endif /* mempool_hpp */
