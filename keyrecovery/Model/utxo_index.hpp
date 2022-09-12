//
//  utxo_index.hpp
//  keyrecovery
//
//  Created by Frederick Benjamin Woodruff on 28/08/2022.
//

#ifndef utxo_index_hpp
#define utxo_index_hpp


#include "transaction.hpp"
#include "blocks.hpp"
#include "base58.hpp"
#include "cached_file_io.hpp"

#include <cstdio>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <queue>

/* ******************************** TYPES ******************************** */



struct utxo_ref : serialisable {
    tx_output out;
    tx_input txhash;
    void append_serial(std::vector<uint8_t>& output) const override;
};
template<>
utxo_ref deserialise(const uint8_t*& start, const uint8_t* end);

template<> struct std::hash<utxo_ref> {
    inline size_t operator()(const utxo_ref & data) const {
        size_t hash = std::hash<tx_output>{}(data.out);
        return hash * 33 + std::hash<tx_input>{}(data.txhash);
    }
};

struct undo_data {
    tx_input input;
    tx_output output;
};

struct tx_index {
    [[TODO]];
    /*
     undos should be files, new directory, name after block to undo, no need to delete on reorgs
     */
    std::string txhash_directory;
    std::string pubkey_directory;
    
    std::deque<std::vector<undo_data>> undos;

    void file_system_overwrite_utxo(tx_input txhash, const std::vector<tx_output>&);
    void file_system_overwrite_utxo(public_key key, const std::vector<utxo_ref>& );
    
    std::vector<tx_output> file_system_fetch_utxo(tx_input txhash) const;
    std::vector<utxo_ref > file_system_fetch_utxo(public_key  key) const;
public:
    

    
    tx_index(std::string txhash_dir, std::string pubkey_dir);
    tx_index() = default;
    tx_index& operator=( tx_index&& other ) = default;
    tx_index( tx_index&& ) = default;
    tx_index( const tx_index& ) = delete;
    tx_index& operator=( const tx_index& other) = delete;
    
    

    void remove_utxo(tx_input txhash, public_key out);
    void add_utxo(tx_input txhash, std::vector<tx_output> out);
    
    void add_block(const block& bl);
    void remove_block(const block& current);
    bool verify_spend(transaction tx) const;
};






/* ******************************** TEMPLATES ******************************** */




#endif /* utxo_index_hpp */
