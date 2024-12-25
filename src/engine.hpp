#pragma once
#include "share.hpp"
#include "word.hpp"

namespace mikan::engine {
struct History {
    std::string raw;
    MeCabWord   converted;
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
    std::vector<History>     histories;
    std::string              tmpdir;

    auto load_configuration() -> bool;
    auto add_history(const History& word) -> bool;
    auto save_hisotry() const -> void;
    auto dump_internal_dict(const char* path) const -> void;
    auto compile_and_reload_user_dictionary() -> void;
    auto reload_dictionary(const char* user_dict = nullptr) -> void;

  public:
    auto convert_wordchain(const WordChain& source, bool best_only, bool ignore_protection = false) const -> WordChains;

    Engine(Share& share);
};
} // namespace mikan::engine
