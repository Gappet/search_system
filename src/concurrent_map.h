#pragma once

#include <cstdlib>
#include <future>
#include <map>
#include <numeric>
#include <random>
#include <string>
#include <vector>
#include <mutex>
#include <vector>



using namespace std::string_literals;

template <typename Key, typename Value>
class ConcurrentMap {
public:
    static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys"s);

    struct Access {
        std::lock_guard<std::mutex> l_g;
        Value& ref_to_value;
        Access(std::mutex& mt, Value& value) : l_g(mt), ref_to_value(value) {}
    };

    explicit ConcurrentMap(size_t bucket_count) : bucket_count_(bucket_count), vector_map_(bucket_count), vector_mutex_(bucket_count) {}

    Access operator[](const Key& key) {
        uint64_t pos = static_cast<uint64_t> (key) % bucket_count_;
        std::lock_guard<std::mutex> l_g(mtx_);
        return Access(vector_mutex_.at(pos), vector_map_[pos][key]);
    }

    std::map<Key, Value> BuildOrdinaryMap() {
        std::map<Key, Value> out;
        for (size_t i = 0; i < bucket_count_; ++i) {
            out.insert(vector_map_[i].begin(), vector_map_[i].end());
        }
        return out;
    }

private:

    size_t bucket_count_;
    std::vector<std::map<Key, Value>> vector_map_;
    std::vector<std::mutex> vector_mutex_;
    std::mutex mtx_;
};