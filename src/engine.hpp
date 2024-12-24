#pragma once
#include <thread>

#include "share.hpp"
#include "util/critical.hpp"
#include "util/event.hpp"
#include "word.hpp"

namespace mikan::engine {
struct History {
    std::string raw;
    MeCabWord   translation;
};

struct FeatureConstriant {
    size_t      begin;
    size_t      end;
    const Word* word;
};

class Engine {
  private:
    Share&                   share;
    std::string              system_dictionary_path;
    std::string              history_file_path;
    std::string              dictionary_compiler_path;
    std::vector<std::string> user_dictionary_paths;
    std::thread              dictionary_updater;
    Event                    dictionary_update_event;
    std::vector<History>     histories;
    Critical<WordChains>     fix_requests;
    std::string              tmpdir;
    bool                     finish_dictionary_updater = false;
    bool                     enable_history            = false;

    auto load_configuration() -> bool;
    auto add_history(const History& word) -> bool;
    auto save_hisotry() const -> void;
    auto dump_internal_dict(const char* path) const -> void;
    auto compile_and_reload_user_dictionary() -> void;
    auto reload_dictionary(const char* user_dict = nullptr) -> void;
    auto recalc_cost(const WordChain& source) const -> long;
    auto compare_and_fix_dictionary(const WordChain& wants) -> void;
    auto dictionary_updater_main() -> void;

  public:
    auto translate_wordchain(const WordChain& source, bool best_only, bool ignore_protection = false) const -> WordChains;
    auto request_fix_dictionary(WordChain wants) -> void;

    Engine(Share& share);
    ~Engine();
};
} // namespace mikan::engine
