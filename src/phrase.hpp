#pragma once

#include <mecab.h>

#include "candidate.hpp"

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
    ProtectionLevel   get_protection_level() const noexcept;
    void              set_protection_level(ProtectionLevel level) noexcept;
    const MeCabWord&  get_raw() const;
    const MeCabWord&  get_translated() const;
    std::string&      get_mutable();
    void              override_translated(const MeCabWord& translated);
    void              override_translated(MeCabWord&& translated);
    bool              is_candidates_initialized() const noexcept;
    bool              has_candidates() const noexcept;
    PhraseCandidates& get_candidates() noexcept;
    void              reset_candidates(PhraseCandidates&& candidates);
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
    bool                     empty() const noexcept;
    size_t                   get_data_size() const noexcept override;
    std::vector<std::string> get_labels() const noexcept override;
    void                     reset(Sentences&& data);
    Phrases*                 get_current_ptr();
    void                     clear();
    Phrases&                 operator[](size_t index);
    SentenceCandidates();
};
