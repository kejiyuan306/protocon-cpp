#pragma once

#include <mutex>
#include <unordered_map>
#include <utility>

namespace Protocon {

template <typename K, typename T>
class ThreadSafeUnorderedMap {
  public:
    void emplace(K&& k, T&& v) {
        std::lock_guard<std::mutex> lock(mtx);
        m.emplace(k, v);
    }

    void erase(const K& k) {
        std::lock_guard<std::mutex> lock(mtx);
        m.erase(k);
    }

    const T& at(const K& k) {
        std::lock_guard<std::mutex> lock(mtx);
        return m.at(k);
    }

  private:
    std::mutex mtx;
    std::unordered_map<K, T> m;
};

}  // namespace Protocon
