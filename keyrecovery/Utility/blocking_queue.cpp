//
//  blocking_queue.cpp
//  keyrecovery
//
//  Created by Frederick Benjamin Woodruff on 28/08/2022.
//


#include "blocking_queue.hpp"

#include <cstdio>
#include <string>

file_guard::file_guard(file_mutex* f, std::string st) {
    owner = f;
    s = st;
    {
        std::unique_lock<std::mutex> lock(owner->d_mutex);
        owner->d_condition.wait(lock, [=]{
            return owner->filenames.find(s) != owner->filenames.end();
        });
        auto [ where, success ] = owner->filenames.insert(s);
        assert(success);
    }
}
file_guard::~file_guard() {
    {
        std::unique_lock<std::mutex> lock(owner->d_mutex);
        owner->filenames.find(s);
    }
    owner->d_condition.notify_all();
}
