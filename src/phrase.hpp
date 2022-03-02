#pragma once
#include <mecab.h>

#include "candidate.hpp"

namespace mikan {
enum class ProtectionLevel {
    None,
    PreserveSeparation,
    PreserveTranslation,
};

class Phrase {
  private:
    PhraseCandidates candidates;
    ProtectionLevel  protection = ProtectionLevel::None;

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
    auto get_candidates() -> PhraseCandidates& {
        return candidates;
    }
    auto reset_candidates(PhraseCandidates&& new_candidates) -> void {
        candidates = std::move(new_candidates);
    }

    Phrase() : candidates(MeCabWord()){};
    Phrase(const std::string& raw) : candidates(MeCabWord(raw)) {}
    Phrase(const MeCab::Node* const node) : candidates(MeCabWord(get_node_surface(node)), node_to_word(node)) {}
    Phrase(MeCabWord&& raw, MeCabWord&& translated) : candidates(MeCabWord(raw), MeCabWord(translated)) {}
};

using Phrases   = std::vector<Phrase>;
using Sentences = std::vector<Phrases>;

class SentenceCandidates : public Candidates {
  private:
    Sentences data;

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
            labels.emplace_back();
            auto& label = labels.back();
            for(auto& p : s) {
                label += p.get_translated().get_feature();
            }
        }
        return labels;
    }
    auto reset(Sentences&& n) -> void {
        data  = std::move(n);
        index = 0;
    }
    auto get_current_ptr() -> Phrases* {
        if(data.empty()) {
            return nullptr;
        }
        return &data[index];
    }
    auto clear() -> void {
        data.clear();
    }

    auto operator[](const size_t index) -> Phrases& {
        return data[index];
    }

    SentenceCandidates() = default;
};
} // namespace mikan
