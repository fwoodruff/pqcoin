//
//  signatures.hpp
//  keyrecovery
//
//  Created by Frederick Benjamin Woodruff on 10/05/2022.
//

#ifndef signatures_hpp
#define signatures_hpp

#include "global.hpp"
#include "falcon.h"

#include <array>
#include <string>
#include <iostream>
#include <vector>
#include <random>
#include <utility>

#define LOGN 10

static thread_local std::once_flag flag;
static thread_local shake256_context cprng;




preimage get_crsn();
void random_bytes(uint8_t* bytes, size_t len);


// pubkey format is 0x10 | (shake(h) --> 32 bytes)
// privkey format 32 random bytes
public_key make_pubkey(private_key privkey);

// crsn is a cryptographically secure random.
// privkey_internal is the compressed [f, g, H] polynomials.
// signature output is 0x3a | r_40 | compress(s2 | s1)
signature sign_message(private_key privkey, preimage crsn, std::vector<uint8_t> message);

// checks first byte is 3a.
// hashes nonce r and message to point.
// verifies s1 and s2 are short, and s2 is invertible
// rebuilds pubkey from s1 and s2, then checks pubkey matches
bool verify_signature(signature sig, public_key pubkey, std::vector<uint8_t> message);

template <> struct std::hash<collision> {
    inline size_t operator()(const collision & data) const {
        size_t hash = 5381;
        for (auto c : data.v) {
                hash = hash * 33 + c; /* hash * 33 + c */
        }
        return hash;
    }
};



std::pair<private_key,public_key> random_keypair();


std::string private_key_to_base58(private_key key);
std::optional<private_key> base58_to_private_key(std::string key);
std::string public_key_to_base58(public_key key);
std::optional<public_key> base58_to_public_key(std::string key);


/*
signature_marker generate_marker(signature sig);
*/
int test_signature_scheme();


#endif /* signatures_hpp */
