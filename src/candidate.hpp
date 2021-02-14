#pragma once
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <fcitx-utils/keysymgen.h>
#include <fcitx/candidatelist.h>

#include "type.hpp"

class Candidates {
  protected:
    uint64_t index;

  public:
    virtual size_t                   get_data_size() const noexcept = 0;
    void                             move_index(int val);
    void                             set_index(uint64_t val);
    uint64_t                         get_index() const noexcept;
    virtual std::vector<std::string> get_labels() const noexcept = 0;
    Candidates() {}
    virtual ~Candidates() {}
};

class MeCabModel;
class PhraseCandidates : public Candidates {
  private:
    MeCabWords data;
    bool is_initialized_with_dictionaries = false;

  public:
    const MeCabWord&         get_raw() const;
    const MeCabWord&         get_translated() const;
    std::string&             get_mutable();
    void                     override_translated(const MeCabWord& translated);
    void                     override_translated(MeCabWord&& translated);
    bool                     is_initialized() const noexcept;
    bool                     has_candidates() const noexcept;
    size_t                   get_data_size() const noexcept override;
    const MeCabWords&        get_data_ref() const noexcept;
    const MeCabWord&         get_current() const;
    std::vector<std::string> get_labels() const noexcept override;
    const MeCabWord&         operator[](size_t index) const;
    PhraseCandidates() {};
    PhraseCandidates(MeCabWord raw);
    PhraseCandidates(MeCabWord raw, MeCabWord translated);
    PhraseCandidates(const std::vector<MeCabModel*>& dictionaries, const std::string& raw, bool best_only);
};

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
    Candidates* const data;
    const size_t      page_size;
    size_t            index_to_local(size_t idx) const noexcept;
    size_t            index_to_global(size_t idx) const noexcept;

    std::vector<std::unique_ptr<CandidateWord>> words;

  public:
    bool match(Candidates* const data) const noexcept;

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

    CandidateList(Candidates* const data, const size_t page_size);
};
