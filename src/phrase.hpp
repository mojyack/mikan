#pragma once
#include "candidate.hpp"

class Phrase {
  public:
    enum class ProtectionLevel {
        NONE,
        PRESERVE_SEPARATION,
        PRESERVE_TRANSLATION,
    };
    std::string      raw;
    std::string      translation;
    ProtectionLevel  protection;
    PhraseCandidates candidates;
    bool             save_to_history = false;
};
using Phrases   = std::vector<Phrase>;
using Sentences = std::vector<Phrases>;

class SentenceCandidates : public Candidates {
  private:
    Sentences data;

    void clear_data() override;

  public:
    size_t                   get_data_size() const noexcept override;
    std::vector<std::string> get_labels() const noexcept override;
    void                     reset(Sentences&& data);
    Phrases*                 get_current_ptr();
    void                     clear() override;
    SentenceCandidates();
};
