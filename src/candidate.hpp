#pragma once
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <fcitx-utils/keysymgen.h>
#include <fcitx/candidatelist.h>

class MeCabModel;
class Candidates {
  private:
    bool                     initialized = false;
    std::optional<uint64_t>  index;
    std::vector<std::string> data;

  public:
    void                            move_index(int val);
    void                            set_index(uint64_t val);
    std::optional<uint64_t>         get_index() const noexcept;
    std::string                     get_current() const;
    const std::vector<std::string>& get_data_ref() const noexcept;
    size_t                          get_data_size() const noexcept;
    bool                            is_initialized() const noexcept;
    bool                            has_candidate() const noexcept;
    void                            clear();
    const std::string               operator[](size_t index) const;
    Candidates() {}
    Candidates(const std::vector<MeCabModel*>& dictionaries, const std::string& phrase, const std::string& first_candidate = std::string());
};

class Phrase;
class CandidateWord : public fcitx::CandidateWord {
  private:
  public:
    void select(fcitx::InputContext* inputContext) const override;
    CandidateWord(fcitx::Text text);
    ~CandidateWord();
};

class CandidateList : public fcitx::CandidateList,
                      public fcitx::PageableCandidateList,
                      public fcitx::CursorMovableCandidateList {
  private:
    Phrase* const phrase;
    const size_t  page_size;
    size_t        index_to_local(size_t idx) const noexcept;
    size_t        index_to_global(size_t idx) const noexcept;

    std::vector<std::unique_ptr<CandidateWord>> words;

  public:
    bool is_phrase(const Phrase* phrase) const noexcept;

    // CandidateList
    const fcitx::Text&   label(int idx) const override;
    const CandidateWord& candidate(int idx) const override;
    int                  cursorIndex() const override;
    int                  size() const override;

    // PageableCandidateList
    bool hasPrev() const override;
    bool hasNext() const override;
    void prev() override;
    void next() override;

    bool usedNextBefore() const override;

    int  totalPages() const override;
    int  currentPage() const override;
    void setPage(int page) override;

    fcitx::CandidateLayoutHint layoutHint() const override;

    // CursorMovableCandidateList
    void prevCandidate() override;
    void nextCandidate() override;

    CandidateList(Phrase* const phrase, const size_t page_size);
};
