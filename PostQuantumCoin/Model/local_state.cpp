//
//  currency.cpp
//  keyrecovery
//
//  Created by Frederick Benjamin Woodruff on 05/09/2022.
//

#include "local_state.hpp"





local_state::local_state(std::string directory) : chain(directory), tree(directory), hist(directory) {
    trusted = 0; 
    live = false;
    
    reboot_info = directory + "trust.bin";
    
}


void persistent_number(uint64_t t, std::string filename) {
    std::vector<uint8_t> tmp;
    serialise_int(t, tmp);
    write_to_file(filename, tmp);
}

std::vector<block> local_state::consume(const block& new_block) {
    using namespace std::chrono_literals;
    auto time = system_clock::now();
    // if not live, we should be querying nodes
    if(chain.top_header().timestamp + 8h < time) {
        live = false;
    }
    if(!new_block.has_signatures() and live) return {};
    bool verified_sigs = new_block.verify_signatures();
    
    if(tree.verify_block(new_block) and (!new_block.has_signatures() or verified_sigs)) {
        hist.add_entry(new_block.h.secure_hash()); 
        tree.consume_block(new_block);
        tree.cleanup(chain.top_header());
        if(chain.fits_on_top(new_block) and new_block.h.verify_time()) {
            if(chain.verify_inputs(new_block) ) {
                chain.push_block(new_block);
                quarantine.evict_shallow(new_block.h.block_height - 1);
                if(verified_sigs) {
                    trusted++;
                    persistent_number(trusted, reboot_info);
                    
                    if(trusted > SIDE_CHAIN_DEPTH) {
                        live = true;
                        return { new_block };
                    }
                } else {
                    trusted = 0;
                    persistent_number(trusted, reboot_info);
                }
                return {};
            }
        } else {
            quarantine.v.push_back(new_block);
            try {
                return reorg();
            } catch(std::runtime_error e) {
                assert(false); // unimplemented
                /*
                 remove everything in utxo_index.
                 
                 
                 */
            }
        }
    }
    return {};
}

void local_state::consume(const signed_transaction& pool_tx) {
    if(!live) return;
    bool succ = chain.verify_spend_mempool(pool_tx);
    if(succ) {
        pool.consume_transaction(pool_tx);
    }
}

uint64_t local_state::balance_for_key(public_key key) const {
    return chain.balance_for_key(key);
}

std::optional<signed_transaction> local_state::generate_tx(private_key sender,
                                              const std::vector<std::pair<public_key, uint64_t>>& recipients,
                                                           uint64_t fee) const {
    return chain.generate_tx(sender, recipients, fee);
}


std::vector<block> linear(std::vector<block>::iterator begin, std::vector<block>::iterator end,
                          const block& top, const collision& bottom) {
    std::vector<block> result;
    auto bl = top;
    result.push_back(top);
    while(bl.h.previous_block_hash != bottom) {
        auto itb = std::find_if(begin, end, [&](const auto& b) {
            return b.h.secure_hash() == bl.h.previous_block_hash;
        });
        assert(itb != end);
        result.push_back(*itb);
    }
    std::reverse(result.begin(), result.end());
    return result;
}


std::vector<block> local_state::reorg() {
    auto time = system_clock::now();
    auto iit = std::partition(quarantine.v.begin(), quarantine.v.end(), [&](const auto& item) {
        return item.h.timestamp <= time;
    });
    
    std::sort(quarantine.v.begin(), iit, [&](const auto& lhs, const auto& rhs) {
        if(lhs.h.block_height == rhs.h.block_height) {
            return lhs.h.timestamp < rhs.h.timestamp;
        }
        return lhs.h.block_height > rhs.h.block_height;
    });
    
    std::vector<block> result;
    
    while(!quarantine.v.empty()) {
        auto bl = quarantine.v.front();
        auto fork = tree.fork_point(bl.h.secure_hash(), chain.top_header().secure_hash());
        
        auto lin_chain = linear(quarantine.v.begin(), iit, bl, *fork);
        if(lin_chain.empty()) {
            return result;
        }
        
        while(true) {
            auto popped_blk = chain.pop_block();
            pool.remove_block(popped_blk);
            quarantine.v.push_back(popped_blk);
            if(popped_blk.h.previous_block_hash == *fork) {
                break;
            }
        }
    
        bool ok = true;
        for(auto bl : lin_chain) {
            if(ok) {
                assert(chain.fits_on_top(bl));
                if(chain.verify_inputs(bl)) {
                    result.push_back(bl);
                    chain.push_block(bl);
                } else {
                    ok = false;
                }
            }
            quarantine.evict_block(bl);
        }
        if(ok) {
            break;
        }
    }
    return result;
}

std::optional<block> local_state::mine(public_key recipient) const {
    milliseconds prev_block_duration = tree.justwhat_duration(chain.top_header());
    block b = pool.generate_unmined_block(recipient, chain.top_header());
    bool val =  mining_attempt(b.h , prev_block_duration);
    if(val) {
        return b;
    }
    return std::nullopt;
}
