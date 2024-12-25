#pragma once
#include <span>

#include "candidates.hpp"
#include "mecab-model.hpp"
#include "mecab-word.hpp"

namespace mikan {
enum class ProtectionLevel {
    None,
    PreserveSeparation,
    PreserveTranslation,
};

struct Word : Candidates {
    // candidates[0]: always present, raw hiragana
    // candidates[1]: automatically converted text
    // candidates[2]...: per-word candidates
    std::vector<std::string> candidates;
    ProtectionLevel          protection = ProtectionLevel::None;

    auto get_data_size() const -> size_t override;
    auto get_labels() const -> std::vector<std::string> override;
    auto has_candidates() const -> bool;
    auto raw() -> std::string&;
    auto raw() const -> const std::string&;
    auto feature() -> std::string&;
    auto feature() const -> const std::string&;

    static auto from_node(const MeCab::Node& node) -> Word;
    static auto from_dictionaries(std::span<MeCabModel*> dicts, const Word& source) -> Word;
    static auto from_raw(std::string raw) -> Word;
};

using WordChain  = std::vector<Word>;
using WordChains = std::vector<WordChain>;
} // namespace mikan
