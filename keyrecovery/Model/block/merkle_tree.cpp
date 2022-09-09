//
//  merkel.cpp
//  keyrecovery
//
//  Created by Frederick Benjamin Woodruff on 09/09/2022.
//

#include "merkle_tree.hpp"
#include "global.hpp"

collision hash_combine(const collision& lhs, const collision& rhs) {
    assert(lhs != rhs);
    collision first, second;
    for(int i = 0; i < lhs.v.size(); i++) {
        if(lhs.v[i] > rhs.v[i]) {
            first.v = lhs.v;
            second.v = rhs.v;
            break;
        }
        if(rhs.v[i] < lhs.v[i]) {
            first.v = rhs.v;
            second.v = lhs.v;
            break;
        }
    }
    uint8_t b = 1;
    assert(lhs != rhs);
    collision out;
    shake256_context ctx;
    shake256_init(&ctx);
    shake256_inject(&ctx, &b, 1);
    shake256_inject(&ctx, first.v.data(), first.v.size());
    shake256_inject(&ctx, second.v.data(), second.v.size());
    shake256_flip(&ctx);
    shake256_extract(&ctx, out.v.data(), out.v.size());
    return out;
}

size_t left_child(size_t t) {
    return 2*t + 1;
}
size_t right_child(size_t t) {
    return 2*t + 2;
}

size_t parent(size_t t) {
    assert(t != 0);
    return (t-1)/2;
}

size_t next_pow_2(size_t t) {
    size_t power = 1;
    while(power < t) {
        power*=2;
    }
    return power;
}


merkle_tree::merkle_tree(const std::vector<collision>& leaves) {
    const auto num_leaf_nodes = next_pow_2(leaves.size());
    const auto buckets = num_leaf_nodes * 2 - 1;
    const auto first_leaf_bucket = buckets - num_leaf_nodes;
    tree.resize(buckets);
    std::copy(leaves.begin(), leaves.end(), tree.begin() + first_leaf_bucket);
    
    *tree[0] = collision{};

    auto idx0 = first_leaf_bucket;
    auto len = num_leaf_nodes/2;
    
    while(len >= 1) {
        idx0 = parent(idx0);
        for(int i = 0; i < len; i ++) {
            const auto& l = tree[left_child(i+idx0)];
            const auto& r = tree[right_child(i+idx0)];
            if(l.has_value() and r.has_value()) {
                tree[i+idx0] = hash_combine(*l, *r);
            } else if (l.has_value()) {
                tree[i+idx0] = l;
            } else {
                tree[i+idx0] = r;
            }
        }
        len = len/2;
    }
}

collision merkle_tree::root() const {
    assert(!tree.empty());
    assert(tree[0].has_value());
    return *tree[0];
}
