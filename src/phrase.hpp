#pragma once
#include <mecab.h>

#include "candidate.hpp"

namespace mikan {
enum class ProtectionLevel {
    NONE,
    PRESERVE_SEPARATION,
    PRESERVE_TRANSLATION,
};

class Phrase {
  private:
    PhraseCandidates candidates;
    ProtectionLevel  protection = ProtectionLevel::NONE;

  public:
    auto get_protection_level() const -> ProtectionLevel;
    auto set_protection_level(ProtectionLevel level) -> void;
    auto get_raw() const -> const MeCabWord&;
    auto get_translated() const -> const MeCabWord&;
    auto get_mutable_feature() -> std::string&;
    auto override_translated(const MeCabWord& translated) -> void;
    auto override_translated(MeCabWord&& translated) -> void;
    auto is_candidates_initialized() const -> bool;
    auto has_candidates() const -> bool;
    auto get_candidates() -> PhraseCandidates&;
    auto reset_candidates(PhraseCandidates&& candidates) -> void;
    Phrase();
    Phrase(const std::string& raw);
    Phrase(MeCabWord&& raw, MeCabWord&& translated);
    Phrase(const MeCab::Node* node);
};

using Phrases   = std::vector<Phrase>;
using Sentences = std::vector<Phrases>;

class SentenceCandidates : public Candidates {
  private:
    Sentences data;

  public:
    auto empty() const -> bool;
    auto get_data_size() const -> size_t override;
    auto get_labels() const -> std::vector<std::string> override;
    auto reset(Sentences&& data) -> void;
    auto get_current_ptr() -> Phrases*;
    auto clear() -> void;
    auto operator[](size_t index) -> Phrases&;

    SentenceCandidates() = default;
};
} // namespace mikan
