#pragma once
#include <fcitx/candidatelist.h>

#include "mecab-word.hpp"

namespace mikan {
class Candidates {
  protected:
    uint64_t index;

  public:
    virtual auto get_data_size() const -> size_t = 0;
    auto         move_index(int val) -> void;
    auto         set_index(uint64_t val) -> void;
    auto         get_index() const -> uint64_t;
    virtual auto get_labels() const -> std::vector<std::string> = 0;

    Candidates()          = default;
    virtual ~Candidates() = default;
};

class MeCabModel;
class PhraseCandidates : public Candidates {
  private:
    MeCabWords data;
    bool       is_initialized_with_dictionaries = false;

  public:
    auto get_raw() const -> const MeCabWord&;
    auto get_translated() const -> const MeCabWord&;
    auto get_mutable_feature() -> std::string&;
    auto override_translated(const MeCabWord& translated) -> void;
    auto override_translated(MeCabWord&& translated) -> void;
    auto is_initialized() const -> bool;
    auto has_candidates() const -> bool;
    auto get_data_size() const -> size_t override;
    auto get_data_ref() const -> const MeCabWords&;
    auto get_current() const -> const MeCabWord&;
    auto get_labels() const -> std::vector<std::string> override;
    auto operator[](size_t index) const -> const MeCabWord&;

    PhraseCandidates() = default;
    PhraseCandidates(MeCabWord raw);
    PhraseCandidates(MeCabWord raw, MeCabWord translated);
    PhraseCandidates(const std::vector<MeCabModel*>& dictionaries, const std::string& raw, bool best_only);
};

class CandidateWord : public fcitx::CandidateWord {
  public:
    auto select(fcitx::InputContext* inputContext) const -> void override;
    CandidateWord(fcitx::Text text);
    ~CandidateWord() = default;
};

class CandidateList : public fcitx::CandidateList,
                      public fcitx::PageableCandidateList,
                      public fcitx::CursorMovableCandidateList {
  private:
    Candidates* const                           data;
    const size_t                                page_size;
    std::vector<std::unique_ptr<CandidateWord>> words;

    auto index_to_local(size_t idx) const -> size_t;
    auto index_to_global(size_t idx) const -> size_t;

  public:
    auto match(Candidates* const data) const -> bool;

    // CandidateList
    auto label(int idx) const -> const fcitx::Text& override;
    auto candidate(int idx) const -> const CandidateWord& override;
    auto cursorIndex() const -> int override;
    auto size() const -> int override;

    // PageableCandidateList
    auto hasPrev() const -> bool override;
    auto hasNext() const -> bool override;
    auto prev() -> void override;
    auto next() -> void override;
    auto usedNextBefore() const -> bool override;
    auto totalPages() const -> int override;
    auto currentPage() const -> int override;
    auto setPage(int page) -> void override;
    auto layoutHint() const -> fcitx::CandidateLayoutHint override;

    // CursorMovableCandidateList
    auto prevCandidate() -> void override;
    auto nextCandidate() -> void override;

    CandidateList(Candidates* const data,size_t page_size);
};
} // namespace mikan
