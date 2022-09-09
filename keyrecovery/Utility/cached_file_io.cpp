//
//  cache.cpp
//  keyrecovery
//
//  Created by Frederick Benjamin Woodruff on 30/08/2022.
//


#include "cached_file_io.hpp"

#include <cstdio>
#include <fstream>
#include <sstream>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>


class cache_entry;



void write_to_file(std::string filename, const std::vector<uint8_t>& bytes) {
    //auto guard = file_system_lock.lock(filename);
    
    std::ofstream file(filename, std::ios::trunc | std::ios_base::binary);
    assert(!file.fail());
    
    std::copy(bytes.begin(), bytes.end(), std::ostreambuf_iterator<char>(file));
}
std::vector<uint8_t> read_file(std::string filename) {
    std::stringstream buffer;
    {
        //auto guard = file_system_lock.lock(filename);
        std::ifstream file(filename, std::ios_base::binary);
        if(!file.good()) { return {}; }
        buffer << file.rdbuf();
    }
    auto str = buffer.str();
    return std::vector<uint8_t>(str.begin(), str.end());
}

bool file_exists(std::string filename) {
    //auto guard = file_system_lock.lock(filename);
    std::ifstream old_file(filename);
    return old_file.good();
}

bool remove_file(std::string filename) {
    //auto guard = file_system_lock.lock(filename);
    bool result;
    {
        std::ifstream old_file(filename);
        result = old_file.good();
    }
    remove(filename.c_str());
    return result;
}



void append_to_file(std::string filename, const std::vector<uint8_t>& bytes ) {
    std::ofstream outfile;
    outfile.open(filename, std::ios_base::app | std::ios_base::binary);
    std::string out;
    out.insert(out.end(), bytes.begin(), bytes.end());
    outfile << out;
}


class database;


cache_entry::cache_entry(database* ctx, std::string filename, bool is_new) {
    m_ctx = ctx;
    m_filename = filename;
    if(!is_new) {
        m_file_data = read_file(m_ctx->m_directory + m_filename);
    }
}
cache_entry::~cache_entry() {
    if(m_file_data.size() == 0) {
        remove_file(m_ctx->m_directory + m_filename);
        return;
    }
    
    if(dirty) {
        write_to_file(m_ctx->m_directory + m_filename, m_file_data);
    }
}
std::vector<uint8_t> cache_entry::get_data() const {
    return m_file_data;
}
void cache_entry::set_data(std::vector<uint8_t> data) {
    dirty = true;
    m_file_data = data;
}


bool make_directory(std::string directory) {
    struct stat st = {0};
    if (stat(directory.c_str(), &st) == -1) {
        if(mkdir(directory.c_str(), S_IRWXU | S_IRWXG | S_IRWXO) == -1) {
            std::cerr << "could not make directory: " << directory << std::endl;
            assert(false);
        }
    }
    return true;
}


database::database(std::string directory, size_t size) :
                    m_directory(directory),
                    KVcache(size, [&](const std::string& ip) { return std::make_shared<cache_entry>(this, ip); }  ) {
    if (file_exists(m_directory + std::string("corrupt"))) {
        throw std::logic_error("database is corrupt");
    }
    write_to_file(m_directory + std::string("corrupt"), {70, 70});
}
    
std::vector<uint8_t> database::read(std::string filename) {
    auto entry = KVcache(filename);
    // if entry count == 1, implies object was held while removed from cache.
    // which would mean that there might have been a new object added with the same filename
    assert(entry.use_count() == 2);
    return entry->get_data();
}
void database::write(std::string filename, std::vector<uint8_t> value) {
    auto entry = KVcache(filename);
    assert(entry.use_count() == 2);
    entry->set_data(value);
}

database::~database() {
    remove_file(m_directory + std::string("corrupt"));
}

bool database::exists(std::string filename) {
    auto entry = KVcache.peek(filename);
    
    if(entry.has_value()) {
        auto value = (*entry)->get_data();
        return !value.empty();
    }
    return file_exists(m_directory + filename);
}

void database::remove_entry(std::string filename) {
    auto entry = KVcache.peek(filename);
    if(entry.has_value()) {
        assert(entry->use_count() == 2);
        (*entry)->set_data({});
    } else {
        remove_file(m_directory + filename);
    }
}
