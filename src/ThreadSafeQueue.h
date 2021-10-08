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
        std::lock_guard<std::mutex> lock(mtx);
        return q.empty();
    }

    const T& front() {
        std::lock_guard<std::mutex> lock(mtx);
        return q.front();
    }

    void emplace(T&& v) {
        std::lock_guard<std::mutex> lock(mtx);
        q.emplace(std::move(v));
    }

    void pop() {
        std::lock_guard<std::mutex> lock(mtx);
        q.pop();
    }

  private:
    std::mutex mtx;
    std::queue<T> q;
};

}  // namespace Protocon
