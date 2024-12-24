#pragma once
#include <mecab.h>

#include "candidate.hpp"

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

class WordChainCandidates : public Candidates {
  private:
    WordChains data;

  public:
    auto empty() const -> bool {
        return data.empty();
    }

    auto get_data_size() const -> size_t {
        return data.size();
    }

    auto get_labels() const -> std::vector<std::string> {
        auto labels = std::vector<std::string>();
        for(auto& s : data) {
            auto& label = labels.emplace_back();
            for(auto& p : s) {
                label += p.get_translated().get_feature();
            }
        }
        return labels;
    }

    auto reset(WordChains&& n) -> void { // FIXME: as value
        data  = std::move(n);
        index = 0;
    }

    auto get_current_ptr() -> WordChain* {
        if(data.empty()) {
            return nullptr;
        }
        return &data[index];
    }

    auto clear() -> void {
        data.clear();
    }

    auto operator[](const size_t index) -> WordChain& {
        return data[index];
    }

    WordChainCandidates() = default;
};
} // namespace mikan
