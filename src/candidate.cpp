#include <fcitx/text.h>
#include <mecab.h>

#include "candidate.hpp"
#include "mecab-model.hpp"
#include "misc.hpp"
#include "type.hpp"

void Candidates::move_index(int val) {
    if(!initialized || !index) {
        return;
    }
    index = (index.value() + val) % data.size();
}
void Candidates::set_index(uint64_t val) {
    if(!initialized || !index) {
        return;
    }
    index = val % data.size();
}
std::optional<uint64_t> Candidates::get_index() const noexcept {
    return index;
}
std::string Candidates::get_current() const {
    if(!initialized || !index) {
        return std::string();
    }
    return data[index.value()];
}
const std::vector<std::string>& Candidates::get_data_ref() const noexcept {
    return data;
}
size_t Candidates::get_data_size() const noexcept {
    return data.size();
}
bool Candidates::is_initialized() const noexcept {
    return initialized;
}
bool Candidates::has_candidate() const noexcept {
    return index.has_value();
}
void Candidates::clear() {
    initialized = false;
    index.reset();
    data.clear();
}
const std::string Candidates::operator[](size_t index) const {
    return data[index];
}
Candidates::Candidates(const std::vector<MeCabModel*>& dictionaries, const std::string& phrase, const std::string& first_candidate) {
    if(!first_candidate.empty()) {
        emplace_unique(data, first_candidate); // first candidate is translated phrase.
    }
    emplace_unique(data, phrase); // second candidate is hiragana.

    for(auto d : dictionaries) {
        auto& lattice = *d->lattice;
        lattice.set_request_type(MECAB_NBEST);
        lattice.set_sentence(phrase.data());
        lattice.set_feature_constraint(0, phrase.size(), "*");
        d->tagger->parse(&lattice);
        while(1) {
            std::string cand = lattice.toString();
            emplace_unique(data, cand);
            if(!lattice.next()) {
                break;
            }
        }

        lattice.set_request_type(MECAB_ONE_BEST);
        d->lattice->clear();
    }
    if(!data.empty()) {
        index = 0;
    }
    initialized = true;
}

void CandidateWord::select(fcitx::InputContext* inputContext) const {
}
CandidateWord::CandidateWord(fcitx::Text text) : fcitx::CandidateWord(std::move(text)) {
}

size_t CandidateList::index_to_local(size_t idx) const noexcept {
    return idx - currentPage() * page_size;
}
size_t CandidateList::index_to_global(size_t idx) const noexcept {
    return idx + currentPage() * page_size;
}
bool CandidateList::is_phrase(const Phrase* comp) const noexcept {
    return phrase == comp;
}
const fcitx::Text& CandidateList::label(int idx) const {
    static fcitx::Text label("");
    return label;
}
const CandidateWord& CandidateList::candidate(int idx) const {
    return *words[index_to_global(idx)];
}
int CandidateList::cursorIndex() const {
    return index_to_local(phrase->candidates.get_index().value());
}
int CandidateList::size() const {
    auto size   = phrase->candidates.get_data_size();
    auto remain = size - currentPage() * page_size;
    return remain > page_size ? page_size : remain;
}
bool CandidateList::hasPrev() const {
    return currentPage() >= 1;
}
bool CandidateList::hasNext() const {
    return currentPage() + 1 < totalPages();
}
void CandidateList::prev() {
    size_t add = page_size;
    if(add > static_cast<size_t>(cursorIndex())) {
        add = cursorIndex();
    }
    phrase->candidates.move_index(-add);
}
void CandidateList::next() {
    size_t add  = page_size;
    auto   size = phrase->candidates.get_data_size();
    if(cursorIndex() + add >= static_cast<size_t>(size)) {
        add = size - 1 - cursorIndex();
    }
    phrase->candidates.move_index(page_size);
}
bool CandidateList::usedNextBefore() const {
    return true;
}
CandidateWord::~CandidateWord() {
}
int CandidateList::totalPages() const {
    auto size = phrase->candidates.get_data_size();
    return size / page_size + 1;
}
int CandidateList::currentPage() const {
    return phrase->candidates.get_index().value() / page_size;
}
void CandidateList::setPage(int page) {
    phrase->candidates.set_index(page * page_size);
}
fcitx::CandidateLayoutHint CandidateList::layoutHint() const {
    return fcitx::CandidateLayoutHint::NotSet;
}
void CandidateList::prevCandidate() {
    phrase->candidates.move_index(-1);
}
void CandidateList::nextCandidate() {
    phrase->candidates.move_index(1);
}
CandidateList::CandidateList(Phrase* const phrase, const size_t page_size) : phrase(phrase), page_size(page_size) {
    setPageable(this);
    setCursorMovable(this);
    for(const auto& w : phrase->candidates.get_data_ref()) {
        words.emplace_back(std::make_unique<CandidateWord>(fcitx::Text(w)));
    }
}
