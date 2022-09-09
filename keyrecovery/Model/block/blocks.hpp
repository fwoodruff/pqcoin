//
//  blocks.hpp
//  keyrecovery
//
//  Created by Frederick Benjamin Woodruff on 13/05/2022.
//

#ifndef blocks_hpp
#define blocks_hpp

#include "global.hpp"
#include "block_header.hpp"
#include "transaction.hpp"
#include "signatures.hpp"

#include <cstdio>
#include <string>
#include <cstdint>
#include <vector>
#include <optional>



struct block : serialisable {
private:
    bool verify_merkle_root() const;
    bool verify_reward() const;
    bool verify_unique_inputs() const;
public:
    
    block_header h;
    std::vector<signed_transaction> txns;
    
    
    void append_serial(std::vector<uint8_t>& output) const override;
    
    block() = default;
    
    bool verify_unsigned(const block_header& latest, uint64_t previous_time) const;
    bool verify_signatures() const;

    
    bool verify_root() const;

    bool has_signatures() const;
    
    void remove_signatures();
};

template<>
block deserialise(const uint8_t*& start, const uint8_t* end);


void write_block(std::string directory, block bloc);
block read_block(std::string directory, collision blockhash);


#endif /* blocks_hpp */
