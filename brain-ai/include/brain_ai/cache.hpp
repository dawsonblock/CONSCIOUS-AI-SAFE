#pragma once
#include <unordered_map>
#include <list>
#include <mutex>
#include <optional>
#include <chrono>

namespace brain_ai {

template<typename K, typename V>
class LRUCache {
public:
    explicit LRUCache(size_t capacity, int ttl_seconds = 300) 
        : capacity_(capacity), ttl_seconds_(ttl_seconds) {}
    
    void put(const K& key, const V& value) {
        std::lock_guard<std::mutex> lock(mu_);
        auto now = std::chrono::steady_clock::now();
        
        auto it = cache_.find(key);
        if (it != cache_.end()) {
            lru_list_.erase(it->second.list_it);
        }
        
        lru_list_.push_front({key, now});
        cache_[key] = {value, lru_list_.begin(), now};
        
        if (cache_.size() > capacity_) {
            evict();
        }
    }
    
    std::optional<V> get(const K& key) {
        std::lock_guard<std::mutex> lock(mu_);
        auto it = cache_.find(key);
        if (it == cache_.end()) {
            return std::nullopt;
        }
        
        auto now = std::chrono::steady_clock::now();
        auto age = std::chrono::duration_cast<std::chrono::seconds>(now - it->second.timestamp).count();
        if (age > ttl_seconds_) {
            cache_.erase(it);
            return std::nullopt;
        }
        
        // Move to front (most recently used)
        lru_list_.erase(it->second.list_it);
        lru_list_.push_front({key, now});
        it->second.list_it = lru_list_.begin();
        it->second.timestamp = now;
        
        return it->second.value;
    }
    
    size_t size() const {
        std::lock_guard<std::mutex> lock(mu_);
        return cache_.size();
    }
    
    void clear() {
        std::lock_guard<std::mutex> lock(mu_);
        cache_.clear();
        lru_list_.clear();
    }
    
private:
    void evict() {
        auto& back = lru_list_.back();
        cache_.erase(back.key);
        lru_list_.pop_back();
    }
    
    struct ListNode {
        K key;
        std::chrono::steady_clock::time_point timestamp;
    };
    
    struct CacheNode {
        V value;
        typename std::list<ListNode>::iterator list_it;
        std::chrono::steady_clock::time_point timestamp;
    };
    
    size_t capacity_;
    int ttl_seconds_;
    mutable std::mutex mu_;
    std::list<ListNode> lru_list_;
    std::unordered_map<K, CacheNode> cache_;
};

} // namespace brain_ai
