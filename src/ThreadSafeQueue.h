#pragma once

#include <mutex>
#include <queue>
#include <utility>

namespace Protocon {

// TODO: We need condition variable here
template <typename T>
class ThreadSafeQueue {
  public:
    bool empty() {
        std::lock_guard<std::mutex> lock(mMtx);
        return mQueue.empty();
    }

    template <typename... Args>
    void emplace(Args&&... args) {
        std::lock_guard<std::mutex> lock(mMtx);
        mQueue.emplace(std::forward<Args>(args)...);
    }

    T pop() {
        std::lock_guard<std::mutex> lock(mMtx);
        T v = mQueue.front();
        mQueue.pop();
        return v;
    }

  private:
    std::mutex mMtx;
    std::queue<T> mQueue;
};

}  // namespace Protocon
