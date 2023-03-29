#pragma once
#include "candidate.hpp"

namespace mikan {
class CandidateWord : public fcitx::CandidateWord {
  public:
    auto select(fcitx::InputContext* inputContext) const -> void override {}
    CandidateWord(fcitx::Text text) : fcitx::CandidateWord(std::move(text)){};
    ~CandidateWord() = default;
};

enum class CandidateListKind {
    CandidateList,
    KanaDisplay,
};

template <class C, class B>
    requires std::derived_from<C, B>
auto downcast(const B* const ptr) -> C* {
    return (C*)((std::byte*)(ptr) - ((std::byte*)(B*)(C*)(0x1000) - (std::byte*)(C*)(0x1000)));
}

class CandidateListBase : public fcitx::CandidateList {
  protected:
    Candidates* const candidates;

  public:
    virtual auto get_kind() const -> CandidateListKind = 0;

    auto get_candidates() const -> const Candidates* {
        return candidates;
    }

    auto layoutHint() const -> fcitx::CandidateLayoutHint override {
        return fcitx::CandidateLayoutHint::NotSet;
    }

    CandidateListBase(Candidates* const candidates) : candidates(candidates) {}
};

class CandidateList : public CandidateListBase,
                      public fcitx::PageableCandidateList,
                      public fcitx::CursorMovableCandidateList {
  private:
    const size_t                                page_size;
    std::vector<std::unique_ptr<CandidateWord>> words;

    auto index_to_local(const size_t idx) const -> size_t {
        return idx - currentPage() * page_size;
    }

    auto index_to_global(const size_t idx) const -> size_t {
        return idx + currentPage() * page_size;
    }

    auto get_kind() const -> CandidateListKind override {
        return CandidateListKind::CandidateList;
    }

  public:
    // CandidateList
    auto label(const int idx) const -> const fcitx::Text& override {
        const static auto label = fcitx::Text("");
        return label;
    }

    auto candidate(const int idx) const -> const CandidateWord& override {
        return *words[index_to_global(idx)];
    }

    auto cursorIndex() const -> int override {
        return index_to_local(candidates->get_index());
    }

    auto size() const -> int override {
        const auto size   = candidates->get_data_size();
        const auto remain = size - currentPage() * page_size;
        return remain > page_size ? page_size : remain;
    }

    // PageableCandidateList
    auto hasPrev() const -> bool override {
        return currentPage() >= 1;
    }

    auto hasNext() const -> bool override {
        return currentPage() + 1 < totalPages();
    }

    auto prev() -> void override {
        auto add = page_size;
        if(add > static_cast<size_t>(cursorIndex())) {
            add = cursorIndex();
        }
        candidates->move_index(-add);
    }

    auto next() -> void override {
        auto       add  = page_size;
        const auto size = candidates->get_data_size();
        if(cursorIndex() + add >= static_cast<size_t>(size)) {
            add = size - 1 - cursorIndex();
        }
        candidates->move_index(page_size);
    }

    auto usedNextBefore() const -> bool override {
        return true;
    }

    auto totalPages() const -> int override {
        const auto size = candidates->get_data_size();
        return size / page_size + 1;
    }

    auto currentPage() const -> int override {
        return candidates->get_index() / page_size;
    }

    auto setPage(int page) -> void override {
        candidates->set_index(page * page_size);
    }

    // CursorMovableCandidateList
    auto prevCandidate() -> void override {
        candidates->move_index(-1);
    }

    auto nextCandidate() -> void override {
        candidates->move_index(1);
    }

    CandidateList(Candidates* const candidates, size_t page_size)
        : CandidateListBase(candidates),
          page_size(page_size) {
        setPageable(this);
        setCursorMovable(this);
        for(const auto& w : candidates->get_labels()) {
            words.emplace_back(std::make_unique<CandidateWord>(fcitx::Text(w)));
        }
    }
};

class KanaDisplay : public CandidateListBase {
  private:
    CandidateWord text;

    auto get_kind() const -> CandidateListKind override {
        return CandidateListKind::KanaDisplay;
    }

  public:
    auto label(const int /*idx*/) const -> const fcitx::Text& override {
        const static auto label = fcitx::Text("");
        return label;
    }

    auto candidate(const int /*idx*/) const -> const CandidateWord& override {
        return text;
    }

    auto cursorIndex() const -> int override {
        return 0;
    }

    auto size() const -> int override {
        return 1;
    }

    KanaDisplay(Candidates* const candidates, std::string text)
        : CandidateListBase(candidates),
          text(fcitx::Text(std::move(text))) {}
};
} // namespace mikan
