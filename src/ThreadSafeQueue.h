#pragma once

#include <mutex>
#include <queue>
#include <utility>

namespace Protocon {

template <typename T>
class ThreadSafeQueue {
  public:
    void emplace(T&& v) {
        std::lock_guard<std::mutex> lock(mtx);
        q.emplace(std::move(v));
    }

    void pop() {
        std::lock_guard<std::mutex> lock(mtx);
        q.pop();
    }

    const T& front() {
        std::lock_guard<std::mutex> lock(mtx);
        return q.front();
    }

  private:
    std::mutex mtx;
    std::queue<T> q;
};

}  // namespace Protocon
