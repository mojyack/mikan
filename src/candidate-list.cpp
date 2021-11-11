#include "candidate.hpp"

namespace mikan {
auto CandidateList::index_to_local(const size_t idx) const -> size_t {
    return idx - currentPage() * page_size;
}
auto CandidateList::index_to_global(const size_t idx) const -> size_t {
    return idx + currentPage() * page_size;
}
auto CandidateList::label(const int idx) const -> const fcitx::Text& {
    const static auto label = fcitx::Text("");
    return label;
}
auto CandidateList::candidate(const int idx) const -> const CandidateWord& {
    return *words[index_to_global(idx)];
}
auto CandidateList::cursorIndex() const -> int {
    return index_to_local(data->get_index());
}
auto CandidateList::size() const -> int {
    const auto size   = data->get_data_size();
    const auto remain = size - currentPage() * page_size;
    return remain > page_size ? page_size : remain;
}
auto CandidateList::hasPrev() const -> bool {
    return currentPage() >= 1;
}
auto CandidateList::hasNext() const -> bool {
    return currentPage() + 1 < totalPages();
}
auto CandidateList::prev() -> void {
    auto add = page_size;
    if(add > static_cast<size_t>(cursorIndex())) {
        add = cursorIndex();
    }
    data->move_index(-add);
}
auto CandidateList::next() -> void {
    auto       add  = page_size;
    const auto size = data->get_data_size();
    if(cursorIndex() + add >= static_cast<size_t>(size)) {
        add = size - 1 - cursorIndex();
    }
    data->move_index(page_size);
}
auto CandidateList::usedNextBefore() const -> bool {
    return true;
}
auto CandidateList::totalPages() const -> int {
    const auto size = data->get_data_size();
    return size / page_size + 1;
}
auto CandidateList::currentPage() const -> int {
    return data->get_index() / page_size;
}
auto CandidateList::setPage(int page) -> void {
    data->set_index(page * page_size);
}
auto CandidateList::layoutHint() const -> fcitx::CandidateLayoutHint {
    return fcitx::CandidateLayoutHint::NotSet;
}
auto CandidateList::prevCandidate() -> void {
    data->move_index(-1);
}
auto CandidateList::nextCandidate() -> void {
    data->move_index(1);
}
CandidateList::CandidateList(Candidates* const data, size_t page_size) : data(data), page_size(page_size) {
    setPageable(this);
    setCursorMovable(this);
    for(const auto& w : data->get_labels()) {
        words.emplace_back(std::make_unique<CandidateWord>(fcitx::Text(w)));
    }
}
} // namespace mikan
