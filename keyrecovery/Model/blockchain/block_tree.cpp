//
//  block_tree.cpp
//  keyrecovery
//
//  Created by Frederick Benjamin Woodruff on 09/09/2022.
//

#include "block_tree.hpp"


void blocktree::cleanup(const block_header& top) {
    
    if(top.block_height % 30 == 0) {
        std::set<collision> longest;
        auto descend = top;
        while(true) {
            auto va = hash_header.find(descend.previous_block_hash);
            if(va == hash_header.end()) {  break; }
            longest.insert(descend.previous_block_hash);
            descend = va->second;
        }

        for(auto it = hash_header.begin(); it != hash_header.end();) {
            auto [hashh, head] = *it;
            if(head.block_height + 2*SIDE_CHAIN_DEPTH < top.block_height) {
                auto hsh = it->second.secure_hash();
                auto blk = read_block(block_directory, hsh);
                [[TODO]];
                // leave removing signatures to history object
                if(longest.find(hsh) != longest.end()) {
                    blk.txns = {};
                } else {
                    blk.remove_signatures();
                }
                write_block(block_directory, blk);
                it = hash_header.erase(it);
            } else {
                it++;
            }
        }
        std::vector<block_header> headers;
        for(const auto& [coll, head] : hash_header) {
            headers.push_back(head);
        }
        std::vector<uint8_t> outs;
        serialise_vec(headers, outs);
        write_to_file(recent_header_file, outs);
    }
}

void blocktree::consume_block(const block& bl) {
    write_block(block_directory, bl);
    auto hah = bl.h.secure_hash();
    hash_header.insert({hah,bl.h});

}

blocktree::blocktree(std::string rootdirectory)  {
    block_directory = rootdirectory + BLOCK_FOLDER;
    make_directory(rootdirectory);
    make_directory(block_directory);
    
    recent_header_file = rootdirectory + "recent_headers.bin";
    
    auto data = read_file(recent_header_file);
    if(!data.empty()) {
        const uint8_t* b = &*data.begin();
        const uint8_t* e = &*data.end();
        auto vec_headers = deserialise_vec<block_header>(b, e);
        
        for(const auto& header : vec_headers) {
            hash_header.insert({header.secure_hash(), header});
        }
    }
}

milliseconds blocktree::justwhat_duration(const block_header& top) const {
    auto it = hash_header.find(top.previous_block_hash);
    assert(it != hash_header.end());
    return duration_cast<milliseconds>(top.timestamp - it->second.timestamp);
}


bool blocktree::verify_block(const block& new_block) const {
    if(hash_header.empty()) {
        return new_block.verify_root();
    }
    // use file here
    // old block
    if(hash_header.find(new_block.h.secure_hash()) != hash_header.end()) {
        return false;
    }
    
    // disconnected block
    auto it = hash_header.find(new_block.h.previous_block_hash);
    if(it == hash_header.end()) {
        return false;
    }

    milliseconds difficulty_time = justwhat_duration(it->second);
    
    bool qual = new_block.verify_unsigned(it->second, difficulty_time);

    return qual;
}


std::optional<collision> blocktree::fork_point(collision lhs, collision rhs) const {
    std::unordered_set<collision> lhs_block_set;
    std::unordered_set<collision> rhs_block_set;
    lhs_block_set.insert(lhs);
    rhs_block_set.insert(rhs);
    for(int i = 0; i < SIDE_CHAIN_DEPTH; i++) {
        if(lhs_block_set.find(rhs) != lhs_block_set.end()) { return rhs; }
        if(rhs_block_set.find(lhs) != rhs_block_set.end()) {  return lhs; }
        auto itl = hash_header.find(lhs);
        
        if(itl != hash_header.end()) {
            lhs = itl->second.previous_block_hash;
            lhs_block_set.insert(lhs);
        }
        auto itr = hash_header.find(rhs);
        if(itr != hash_header.end()) {
            rhs = itr->second.previous_block_hash;
            rhs_block_set.insert(rhs);
        }
        if(itl == hash_header.end() and itr == hash_header.end()) {
            return std::nullopt;
        }
    }
    return std::nullopt;
}
