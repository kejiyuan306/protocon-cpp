#pragma once

#include <mutex>
#include <queue>
#include <utility>

namespace Protocon {

// TODO: 我们可能需要条件变量
template <typename T>
class ThreadSafeQueue {
  public:
    bool empty() {
        std::lock_guard<std::mutex> lock(mMtx);
        return mQueue.empty();
    }

    const T& front() {
        std::lock_guard<std::mutex> lock(mMtx);
        return mQueue.front();
    }

    void emplace(T&& v) {
        std::lock_guard<std::mutex> lock(mMtx);
        mQueue.emplace(std::move(v));
    }

    void pop() {
        std::lock_guard<std::mutex> lock(mMtx);
        mQueue.pop();
    }

  private:
    std::mutex mMtx;
    std::queue<T> mQueue;
};

}  // namespace Protocon
