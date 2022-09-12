//
//  queue.hpp
//  https_server
//
//  Created by Frederick Benjamin Woodruff on 23/05/2022.
//

#ifndef queue_hpp
#define queue_hpp


#include <mutex>
#include <condition_variable>
#include <deque>
#include <optional>
#include <unordered_set>



class file_mutex;

class file_guard {
    file_mutex* owner;
    std::string s;
public:
    file_guard(const file_guard&) = delete;
    file_guard& operator=(const file_guard&) = delete;
    file_guard(file_mutex* f, std::string s);
    ~file_guard();
};

class file_mutex {
    std::mutex d_mutex;
    std::condition_variable d_condition;
    std::unordered_set<std::string> filenames;
public:
    [[nodiscard]] file_guard lock(std::string filename) {
        return file_guard(this, filename);
    }
    friend class file_guard;
    
};



template <typename T>
class blocking_queue {
private:
    std::mutex              d_mutex;
    std::condition_variable d_condition;
    std::deque<T>           d_queue;
public:
    void push(T const& value) {
        {
            std::unique_lock<std::mutex> lock(this->d_mutex);
            d_queue.push_front(value);
        }
        this->d_condition.notify_one();
    }
    T pop() {
        std::unique_lock<std::mutex> lock(this->d_mutex);
        this->d_condition.wait(lock, [=]{ return !this->d_queue.empty(); });
        T rc(std::move(this->d_queue.back()));
        this->d_queue.pop_back();
        return rc;
    }
    
    std::optional<T> try_pop() {
        std::unique_lock<std::mutex> lock(this->d_mutex);
        if(this->d_queue.empty()) {
            return std::nullopt;
        }
        T rc(std::move(this->d_queue.back()));
        this->d_queue.pop_back();
        return rc;
    }
};

#endif /* queue_hpp */
