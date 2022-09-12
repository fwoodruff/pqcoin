//
//  merkel.hpp
//  keyrecovery
//
//  Created by Frederick Benjamin Woodruff on 09/09/2022.
//

#ifndef merkel_hpp
#define merkel_hpp

#include "global.hpp"

#include <cstdio>
#include <vector>
#include <optional>



struct merkle_tree {
    std::vector<std::optional<collision>> tree {};
public:
    merkle_tree(const std::vector<collision>& leaves);
    collision root() const;
};



#endif /* merkel_hpp */
