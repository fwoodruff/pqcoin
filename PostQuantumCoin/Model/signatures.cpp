//
//  signatures.cpp
//  keyrecovery
//
//  Created by Frederick Benjamin Woodruff on 10/05/2022.
//

#include "signatures.hpp"
#include "falcon.h"
#include "global.hpp"
#include "base58.hpp"

#include <array>
#include <string>
#include <iostream>
#include <vector>
#include <cassert>
#include <optional>
#include <unordered_set>


constexpr int PRIVATE_KEY_SIZE = FALCON_PRIVKEY_SIZE(LOGN);
constexpr int SIGNATURE_SIZE = FALCON_SIG_COMPRESSED_MAXSIZE(LOGN) * 2;
constexpr int KEY_GEN_BUFFER_SIZE = FALCON_TMPSIZE_KEYGEN(LOGN);

constexpr int SIGN_DYNAMIC_BUFFER_SIZE = FALCON_TMPSIZE_SIGNDYN(LOGN) + 2048;
constexpr int VERIFY_BUFFER_SIZE = FALCON_TMPSIZE_VERIFY(LOGN) + 2048;


std::string private_key_to_base58(private_key key) {
    return encode_base58_checked<PREIMAGE_SIZE>(key.v);
}
std::optional<private_key> base58_to_private_key(std::string key) {
    return decode_base58_checked<PREIMAGE_SIZE>(key);
}
std::string public_key_to_base58(public_key key) {
    return encode_base58_checked<COLLISION_SIZE>(key.v);
}
std::optional<public_key> base58_to_public_key(std::string key) {
    auto v = decode_base58_checked<COLLISION_SIZE>(key);
    if(v == std::nullopt) return std::nullopt;
    public_key result;
    std::copy(v->begin(), v->end(), result.v.begin());
    return { result };
}


private_key random_private() {
    private_key result {};
    random_bytes(result.v.data(), result.v.size());
    return result;
}

// pubkey format is (shake(h) --> 48 bytes)
// privkey format 32 random bytes

public_key make_pubkey(private_key privkey) {
    std::array<uint8_t, KEY_GEN_BUFFER_SIZE> key_gen_buffer {};
    shake256_context key_rng;
    shake256_init_prng_from_seed(&key_rng, privkey.v.data(), privkey.v.size());
    std::array<uint8_t, PRIVATE_KEY_SIZE> privkey_internal {};
    public_key pubkey {};
    int err_keygen = falcon_keygen_make(
        &key_rng,
        LOGN,
        privkey_internal.data(), privkey_internal.size(),
        pubkey.v.data(), pubkey.v.size(),
        key_gen_buffer.data(), key_gen_buffer.size());
    
    assert(err_keygen == 0);
    
    
    return { pubkey };
}



std::pair<private_key,public_key> random_keypair() {
    auto sk = random_private();
    auto pk = make_pubkey(sk);
    return {sk, pk};
}

// crsn is a cryptographically secure random.
// privkey_internal is the compressed [f, g, H] polynomials.
// signature output is 0x3a | r_40 | compress(s2 | s1)
signature sign_message(private_key privkey, preimage crsn, std::vector<uint8_t> message) {
    // if a misguided transaction signer uses a different signature algorithm to their keygen, for the
    // same private key, the transaction will be discarded and they can retry with the right signature algorithm
    // therefore marking public keys is unnecessary
    std::array<uint8_t, SIGN_DYNAMIC_BUFFER_SIZE> tmp_buffer_sd {};
    std::array<uint8_t, KEY_GEN_BUFFER_SIZE>  tmp_buffer_kg {};
    
    shake256_context key_rng;
    shake256_init_prng_from_seed(&key_rng, privkey.v.data(), privkey.v.size());
    std::array<uint8_t, PRIVATE_KEY_SIZE> privkey_internal {};
    std::array<uint8_t, COLLISION_SIZE> pubkey {};
    
    int err_keygen = falcon_keygen_make(
        &key_rng,
        LOGN,
        privkey_internal.data(), privkey_internal.size(),
        pubkey.data(), pubkey.size(),
        tmp_buffer_kg.data(), tmp_buffer_kg.size());
    
    assert(err_keygen == 0);
    
    shake256_context sign_rng;
    shake256_init_prng_from_seed(&sign_rng, crsn.v.data(), crsn.v.size());
    
    std::array<uint8_t, SIGNATURE_SIZE> signature {};
    size_t sigsize = signature.size();

    int err_sign = falcon_sign_dyn(&sign_rng,
                                   signature.data(), &sigsize, FALCON_SIG_COMPRESSED,
                                   privkey_internal.data(), privkey_internal.size(),
                                   message.data(), message.size(),
                                   tmp_buffer_sd.data(), tmp_buffer_sd.size());
    
    assert(err_sign == 0);
    
    std::vector<uint8_t> out;
    out.insert(out.end(), signature.begin(), signature.begin()+sigsize);
    
    return { out };
}


// checks first byte is 3a.
// hashes nonce r and message to point.
// verifies s1 and s2 are short, and s2 is invertible
// rebuilds pubkey from s1 and s2, then checks pubkey matches
bool verify_signature(signature signature, public_key pubkey, std::vector<uint8_t> message) {
    
    if(signature.v.size() < 1) {
        return false;
    }
    if(!(signature.v[0] == 0x3a or signature.v[0] == 0x39)) {
        // if nerds want to attack themselves, they may.
        return false;
    }
    
    std::array<uint8_t, VERIFY_BUFFER_SIZE> tmp_buffer_v {};
    int err_verify = falcon_verify(signature.v.data(), signature.v.size(), FALCON_SIG_COMPRESSED,
                                   pubkey.v.data(), pubkey.v.size(),
                                   message.data(), message.size(),
                                   tmp_buffer_v.data(), tmp_buffer_v.size());
    
    if(err_verify == 0) {
        return true;
    }
    return false;
}

void random_bytes(uint8_t* bytes, size_t len) {
    constexpr size_t SEED_SIZE = 64;
    std::call_once(flag, [&]() {
        std::array<uint8_t, SEED_SIZE> seed;
        std::random_device rd;
        for(auto & v : seed) {
            v = rd();
        }
        shake256_init_prng_from_seed(&cprng, seed.data(), SEED_SIZE);
    });
    shake256_extract(&cprng, bytes, len);
}

preimage get_crsn() {
    std::array<uint8_t, PREIMAGE_SIZE> out;
    random_bytes(out.data(), out.size());
    return { out };
}





int test_signature_scheme() {
    shake256_context sc;
    uint8_t seed[5] = {0,1,2,3,4};
    shake256_init_prng_from_seed(&sc, seed, 5);
    
    for(int i = 0; i < 10; i++) {
        private_key privkey {};
        shake256_extract(&sc, privkey.v.data(), PREIMAGE_SIZE);
        
        auto pubkey = make_pubkey(privkey);

        std::vector<uint8_t> message;
        message.resize(500);
        shake256_extract(&sc, message.data(), 500);
        
        
        preimage crsn {};
        shake256_extract(&sc, crsn.v.data(), COLLISION_SIZE);
        
        auto signature = sign_message(privkey, crsn, message);
        std::cout << signature.v.size() << ": ";
        
        for(size_t i = 0; i < signature.v.size(); i += 10) {
            std::cout << std::hex << unsigned( signature.v[i]) << " ";
        }
        std::cout << "\n" <<std::endl;
        
        bool ver = verify_signature(signature, pubkey, message);
        
        auto badsig = signature;
        badsig.v[i] = badsig.v[i] + 1;
        bool fal = verify_signature(badsig, pubkey, message);
        
        auto badmsg = message;
        badmsg[i] = badmsg[i] + 1;
        bool fal2 = verify_signature(signature, pubkey, badmsg);
        
        
        
        assert(ver);
        assert(!fal);
        assert(!fal2);
    }
    return 0;
}

/*
signature_marker generate_marker(signature sig) {
    signature_marker out;
    
    shake256_context hs;
    shake256_init_prng_from_seed(&hs, sig.v.data(), sig.v.size());
    shake256_extract(&hs, out.v.data(), out.v.size());
    return out;
}
*/







