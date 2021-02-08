#pragma once
#include <condition_variable>
#include <mutex>
#include <string>

#include <fcitx-utils/keysym.h>

#include "candidate.hpp"
#include "configuration.hpp"

// utility
template <typename T>
struct SafeVar {
    std::mutex mutex;
    T          data;
    void       store(T src) {
        std::lock_guard<std::mutex> lock(mutex);
        data = src;
    }
    T load() {
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
    template <typename T>
    bool wait_for(T duration) {
        waked.store(false);
        std::unique_lock<std::mutex> lock(waked.mutex);
        return condv.wait_for(lock, duration, [this]() { return waked.data; });
    }
    void wakeup() {
        waked.store(true);
        condv.notify_all();
    }
};

// mikan
class Share {
  public:
    int                      candidate_page_size;
    size_t                   auto_commit_threshold;
    SafeVar<MeCabModel*>     primary_vocabulary    = nullptr;
    MeCabModel*              additional_vocabulary = nullptr;
    KeyConfig                key_config;
};

class Phrase {
  public:
    enum class ProtectionLevel {
        NONE,
        PRESERVE_SEPARATION,
        PRESERVE_TRANSLATION,
    };
    std::string     raw;
    std::string     translation;
    ProtectionLevel protection;
    Candidates      candidates;
};
