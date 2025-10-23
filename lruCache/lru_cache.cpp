#include "./lru_cache.hpp"
#include<string>
#include<iostream>
using namespace std;

LRUCache :: LRUCache(int cap){
    capacity_ = cap;
}

string LRUCache :: get(int key){
    auto it = map_.find(key);
    //item not found
    if(it == map_.end()){
        return "";
    }
    //if item is found, put at front
    items_.splice(items_.begin(),items_,it->second);
    //return item
    return it->second->second;
}

void LRUCache :: put(int key, const string &value){
    if(capacity_<=0) return;

    auto it = map_.find(key);
    //item found, move to front and update value
    if(it!=map_.end()){
        items_.splice(items_.begin(),items_,it->second);
        it->second->second = value;
        return;
    }

    //delete least recently used item(back of list) if full and insert new item to front
    if(items_.size()==capacity_){
        int key_remove = items_.back().first;
        items_.pop_back();
        map_.erase(key_remove);
    }
    items_.push_front({key,value});
    map_[key] = items_.begin();
    return;
}