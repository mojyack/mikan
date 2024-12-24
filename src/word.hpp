#pragma once
#include <mecab.h>

#include "word-candidates.hpp"

namespace mikan {
enum class ProtectionLevel {
    None,
    PreserveSeparation,
    PreserveTranslation,
};

class Word {
  private:
    WordCandidates  candidates;
    ProtectionLevel protection = ProtectionLevel::None;

    static auto get_node_surface(const MeCab::Node* const node) -> std::string {
        return std::string(node->surface, node->length);
    }

    static auto node_to_word(const MeCab::Node* const node) -> MeCabWord {
        if(node->stat == MECAB_UNK_NODE) {
            return MeCabWord(get_node_surface(node));
        } else {
            return MeCabWord(node->feature, node->rcAttr, node->lcAttr, node->wcost, node->cost, true);
        }
    }

  public:
    auto get_protection_level() const -> ProtectionLevel {
        return protection;
    }

    auto set_protection_level(ProtectionLevel level) -> void {
        protection = level;
    }

    auto get_raw() const -> const MeCabWord& {
        return candidates.get_raw();
    }

    auto get_translated() const -> const MeCabWord& {
        return candidates.get_translated();
    }

    auto get_mutable_feature() -> std::string& {
        return candidates.get_mutable_feature();
    }

    auto override_translated(const MeCabWord& translated) -> void {
        candidates.override_translated(translated);
    }

    auto override_translated(MeCabWord&& translated) -> void {
        candidates.override_translated(translated);
    }

    auto is_candidates_initialized() const -> bool {
        return candidates.is_initialized();
    }

    auto has_candidates() const -> bool {
        return candidates.has_candidates();
    }

    auto get_candidates() -> WordCandidates& {
        return candidates;
    }

    auto reset_candidates(WordCandidates&& new_candidates) -> void {
        candidates = std::move(new_candidates);
    }

    Word() : candidates(MeCabWord()) {};

    Word(const std::string& raw) : candidates(MeCabWord(raw)) {}

    Word(const MeCab::Node* const node) : candidates(MeCabWord(get_node_surface(node)), node_to_word(node)) {}

    Word(MeCabWord&& raw, MeCabWord&& translated) : candidates(MeCabWord(raw), MeCabWord(translated)) {}
};

using WordChain  = std::vector<Word>;
using WordChains = std::vector<WordChain>;
} // namespace mikan
