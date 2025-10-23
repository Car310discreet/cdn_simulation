#ifndef LRU_CACHE_H
#define LRU_CACHE_H

#include <string>
#include <list>
#include <unordered_map>
#include <utility>
using namespace std;

class LRUCache
{
private:
    int capacity_;
    list<pair<int, string>> items_;
    unordered_map<int, list<pair<int, string>>::iterator> map_;

public:
    explicit LRUCache(int capacity);
    string get(int key);
    void put(int key, const string &value);
};

#endif // LRU_CACHE_H