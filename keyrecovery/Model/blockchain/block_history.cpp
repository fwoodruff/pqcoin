//
//  block_history.cpp
//  keyrecovery
//
//  Created by Frederick Benjamin Woodruff on 09/09/2022.
//

#include "block_history.hpp"
#include "cached_file_io.hpp"

history::history(std::string directory) {
    collision_history_file = directory + "hashes.bin";
    
    std::ifstream file(collision_history_file);
    collision v;
    while ( file.read(reinterpret_cast<char*>(v.v.data()), v.v.size()) ) {
        hist.push_back(v);
    }
}

void history::add_entry(collision coll) {
    hist.push_back(coll);
    entry.insert({coll, hist.size() - 1});
    
    append_to_file(collision_history_file, coll.serialise() );
}


size_t history::fetch_index(collision coll) {
    auto val = entry.find(coll);
    assert(entry.end() != val);
    return val->second;
}
