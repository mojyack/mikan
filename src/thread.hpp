#pragma once
#include <condition_variable>
#include <functional>
#include <mutex>

namespace mikan {
template <typename T>
struct SafeVar {
    mutable std::mutex mutex;
    T                  data;

    void store(T src) {
        std::lock_guard<std::mutex> lock(mutex);
        data = src;
    }
    T load() const {
        std::lock_guard<std::mutex> lock(mutex);
        return data;
    }
    SafeVar(T src) : data(src) {}
    SafeVar() {}
};

class ConditionalVariable {
  private:
    std::condition_variable condv;
    SafeVar<bool>           waked;

  public:
    void wait() {
        waked.store(false);
        std::unique_lock<std::mutex> lock(waked.mutex);
        condv.wait(lock, [this]() { return waked.data; });
    }
    template <typename D>
    bool wait_for(D duration) {
        waked.store(false);
        std::unique_lock<std::mutex> lock(waked.mutex);
        return condv.wait_for(lock, duration, [this]() { return waked.data; });
    }
    void wakeup() {
        waked.store(true);
        condv.notify_all();
    }
};
} // namespace xrun
