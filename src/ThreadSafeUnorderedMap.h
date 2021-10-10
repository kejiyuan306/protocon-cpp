#pragma once

#include <mutex>
#include <unordered_map>
#include <utility>

namespace Protocon {

template <typename K, typename T>
class ThreadSafeUnorderedMap {
  public:
    template <typename... Args>
    void emplace(Args&&... args) {
        std::lock_guard<std::mutex> lock(mMtx);
        mMap.emplace(std::forward<Args>(args)...);
    }

    K erase(const K& k) {
        std::lock_guard<std::mutex> lock(mMtx);
        return *mMap.erase(k);
    }

    const T& at(const K& k) {
        std::lock_guard<std::mutex> lock(mMtx);
        return mMap.at(k);
    }

  private:
    std::mutex mMtx;
    std::unordered_map<K, T> mMap;
};

}  // namespace Protocon
