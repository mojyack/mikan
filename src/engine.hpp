#pragma once
#include "share.hpp"
#include "word.hpp"

namespace mikan::engine {
struct FeatureConstriant {
    size_t      begin;
    size_t      end;
    const Word* word;
};

struct ConvDef {
    std::string raw;
    std::string converted;
};

class Engine {
  private:
    Share&                   share;
    std::string              system_dictionary_path;
    std::string              history_file_path;
    std::string              dictionary_compiler_path;
    std::vector<std::string> user_dictionary_paths;

    auto parse_configuration_line(std::string_view line) -> bool;
    auto load_configuration() -> bool;
    auto merge_dictionaries(const char* path) const -> bool;

  public:
    auto compile_and_reload_user_dictionary() -> bool;
    auto reload_dictionary(const char* user_dict = nullptr) -> bool;
    auto convert_wordchain(const WordChain& source, bool best_only, bool ignore_protection = false) const -> WordChains;
    auto add_convert_definition(std::string_view raw, std::string_view converted) -> bool;
    auto remove_convert_definition(std::string_view raw) -> bool;

    Engine(Share& share);
};
} // namespace mikan::engine
