//
//  block_cache.hpp
//  keyrecovery
//
//  Created by Frederick Benjamin Woodruff on 16/05/2022.
//

#ifndef block_cache_hpp
#define block_cache_hpp


#include <list>
#include <unordered_map>
#include <cassert>
#include <memory>
#include <string>
#include <vector>



void write_to_file(std::string filename, const std::vector<uint8_t>& bytes);
std::vector<uint8_t> read_file(std::string filename);
bool file_exists(std::string filename);
bool remove_file(std::string filename);

void append_to_file(std::string filename, const std::vector<uint8_t>& bytes );

bool make_directory(std::string directory);

/*

 // caching strategy: always keep most recent
 for given blockhash need previous. Can determine by loading a file and hashing contents.
 
 // caching strategy: always keep most recent
 pubkey bloom filter for each blockhash -> cached. Can determine by loading a block, and mapping
 
 //  caching strategy: least recently used
 blockhash to block -> cached, can determine by loading a block by name.
 
 */
//LRU Cache
#include <cassert>
#include <list>
#include <unordered_map>


// always returns value
template <typename K, typename V>
class LRUCache {
    std::list<K> item_list;
    std::unordered_map<K, std::pair<V, typename std::list<K>::iterator>> item_map;
    using Fn = std::function<V(const K&)>;
    Fn call;
    size_t capacity;
public:
    LRUCache( size_t capacity_, Fn call_ ) : call( call_ ), capacity( capacity_ ) { assert(capacity != 0); }

    V operator()( const K& key ) {

        auto pair_it = item_map.find( key );
        if( pair_it != item_map.end() ) {
            item_list.splice( item_list.end(), item_list, pair_it->second.second );
            return pair_it->second.first;
        }
        V val = call( key );
        if( item_list.size() == capacity ) {
            
            auto front = item_map.find( item_list.front() );
            item_map.erase( front );
            item_list.pop_front();
        }
        auto key_it = item_list.insert( item_list.end(), key );
        item_map.insert( { key, { val, key_it } });
        return val;
        
    }
    
    std::optional<V> peek( const K& key) {
        auto it = item_map.find(key);
        if(it == item_map.end()) {
            return std::nullopt;
        }
        return it->second.first;
    }
    
    // peek function that doesn't move to front
    
    bool exists(const K& key) {
        auto pair_it = item_map.find( key );
        return pair_it != item_map.end();
    }
    

};

template <typename K, typename V = K>
class StackCache {
    std::list<K> item_list;
    std::unordered_map<K, V> item_map;
    size_t capacity;
    
    StackCache(size_t capacity_) : capacity( capacity_ ) { assert(capacity != 0); }
public:
    void push(K key, V val) {
        if( item_list.size() == capacity ) {
            auto it = item_map.find( item_list.front() );
            item_map.erase( it );
            item_list.pop_front();
        }
        auto key_it = item_list.insert( item_list.end(), key );
        item_map.insert( { key, { val, key_it } } );
    }
    
    std::optional<V> operator()( const K& key ) const {
        
        auto it = item_map.find( key );
        if( it != item_map.end() ) {
            //return it->second.first;
        }
        return std::nullopt;
    }
    
    void clear() {
        item_list.clear();
        item_map.clear();
    }
};



class database;

class cache_entry {
    database* m_ctx;
    bool dirty = false;
    std::vector<uint8_t> m_file_data;
    std::string m_filename;
public:
    cache_entry(database* ctx, std::string inp, bool is_new = false);
    ~cache_entry();
    cache_entry(const cache_entry & other) = delete;
    cache_entry& operator=(const cache_entry & other) = delete;
    std::vector<uint8_t> get_data() const;
    void set_data(std::vector<uint8_t> new_outputs);
};

class database {
    LRUCache<std::string, std::shared_ptr<cache_entry>> KVcache;
    std::string m_directory;
public:
    friend class cache_entry;
    database(std::string directory, size_t size = 500);
    ~database();
    database(const database & other) = delete;
    database& operator=(const database & other) = delete;

    
    std::vector<uint8_t> read(std::string filename);
    void write(std::string filename, std::vector<uint8_t> file_data);
    
    bool exists(std::string filename);
    void remove_entry(std::string filename);
};








#endif /* block_cache_hpp */
