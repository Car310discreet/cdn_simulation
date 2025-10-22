#ifndef LRU_CACHE_H
#define LRU_CACHE_H

#include <string>
#include <list>
#include <unordered_map>
#include <utility>

class LRUCache
{
private:
    int capacity_;
    std::list<std::pair<int, std::string>> items_;
    std::unordered_map<int, std::list<std::pair<int, std::string>>::iterator> map_;

public:
    explicit LRUCache(int capacity);
    std::string get(int key);
    void put(int key, const std::string &value);
};

#endif // LRU_CACHE_H