#pragma once
#include <condition_variable>
#include <list>
#include <mutex>
#include <vector>

template <typename T>
struct SafeVar {
    mutable std::mutex mutex;
    T                  data;

    void store(T src) {
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

class MeCabWord {
  private:
    std::string    feature;
    bool           is_word_info_valid = false;
    unsigned short cattr_right = 0;
    unsigned short cattr_left = 0;
    short          word_cost = 0;
    long           total_cost = 0;

  public:
    const std::string& get_feature() const noexcept {
        return feature;
    }
    std::string& get_mutable() noexcept {
        return feature;
    }
    bool has_valid_word_info() const noexcept {
        return is_word_info_valid;
    }
    bool has_same_attr(const MeCabWord& o) const {
        return is_word_info_valid && o.cattr_right == cattr_right && o.cattr_left == cattr_left;
    }
    unsigned short get_cattr_right() const noexcept {
        return cattr_right;
    }
    unsigned short get_cattr_left() const noexcept {
        return cattr_left;
    }
    short get_word_cost() const noexcept {
        return is_word_info_valid ? word_cost : 0;
    }
    void override_word_cost(short cost) noexcept {
        word_cost = cost;
    }
    long get_total_cost() const noexcept {
        return is_word_info_valid ? total_cost : 0;
    }
    MeCabWord(const std::string& feature) : feature(feature) {
    }
    MeCabWord(const std::string& feature, short cattr_right, short cattr_left, short word_cost, long total_cost, bool from_system_dic) : feature(feature),
                                                                                                                                         is_word_info_valid(true),
                                                                                                                                         cattr_right(from_system_dic ? cattr_right : 0),
                                                                                                                                         cattr_left(from_system_dic ? cattr_left : 0),
                                                                                                                                         word_cost(from_system_dic ? word_cost : 0),
                                                                                                                                         total_cost(from_system_dic ? total_cost : 0) {
    }
    MeCabWord() {
    }
};
using MeCabWords = std::vector<MeCabWord>;
struct History {
    std::string raw;
    MeCabWord   translation;
};
using Histories = std::vector<History>;
