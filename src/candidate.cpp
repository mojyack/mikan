#include <algorithm>

#include <fcitx/text.h>
#include <mecab.h>

#include "candidate.hpp"
#include "mecab-model.hpp"
#include "misc.hpp"
#include "type.hpp"

void Candidates::move_index(int val) {
    index = (index + val) % get_data_size();
}
void Candidates::set_index(uint64_t val) {
    index = val % get_data_size();
}
uint64_t Candidates::get_index() const noexcept {
    return index;
}

const MeCabWord& PhraseCandidates::get_raw() const {
    if(data.size() < 2) {
        return get_translated();
    } else {
        return data[1];
    }
}
const MeCabWord& PhraseCandidates::get_translated() const {
    return data[index];
}
std::string& PhraseCandidates::get_mutable() {
    return data[data.size() < 2 ? 0 : 1].get_mutable();
}
void PhraseCandidates::override_translated(const MeCabWord& translated) {
    data[0] = translated;
}
void PhraseCandidates::override_translated(MeCabWord&& translated) {
    data[0] = std::move(translated);
}
bool PhraseCandidates::has_candidates() const noexcept {
    return data.size() > 1;
}
bool PhraseCandidates::is_initialized() const noexcept {
    return is_initialized_with_dictionaries;
}
const MeCabWord& PhraseCandidates::get_current() const {
    return data[index];
}
const MeCabWords& PhraseCandidates::get_data_ref() const noexcept {
    return data;
}
size_t PhraseCandidates::get_data_size() const noexcept {
    return data.size();
}
std::vector<std::string> PhraseCandidates::get_labels() const noexcept {
    std::vector<std::string> labels;
    for(auto& d : data) {
        labels.emplace_back(d.get_feature());
    }
    return labels;
}
const MeCabWord& PhraseCandidates::operator[](size_t index) const {
    return data[index];
}
PhraseCandidates::PhraseCandidates(MeCabWord raw) {
    data  = MeCabWords{std::move(raw)};
    index = 0;
}
PhraseCandidates::PhraseCandidates(MeCabWord raw, MeCabWord translated) {
    data  = MeCabWords{std::move(translated), std::move(raw)};
    index = 0;
}
PhraseCandidates::PhraseCandidates(const std::vector<MeCabModel*>& dictionaries, const std::string& raw, bool best_only) {
    const auto predicate_same_feature = [](const MeCabWord& elm, const MeCabWord& o) -> bool {
        return elm.get_feature() == o.get_feature();
    };

    for(auto d : dictionaries) {
        auto& lattice = *d->lattice;
        lattice.set_request_type(best_only ? MECAB_ONE_BEST : MECAB_NBEST);
        lattice.set_sentence(raw.data());
        lattice.set_feature_constraint(0, raw.size(), "*");
        d->tagger->parse(&lattice);
        while(1) {
            for(const auto* node = lattice.bos_node(); node; node = node->next) {
                if(node->stat != MECAB_NOR_NODE) {
                    continue;
                }
                emplace_unique(data, MeCabWord(node->feature, node->rcAttr, node->lcAttr, node->wcost, node->cost, d->is_system_dictionary), predicate_same_feature);
            }
            if(!lattice.next()) {
                break;
            }
        }
        lattice.clear();
    }

    // second candidate is hiragana.
    MeCabWord hiragana;
    if(auto p = std::find_if(data.begin(), data.end(), [&raw](const MeCabWord& o) {
           return o.get_feature() == raw;
       }); p != data.end()) {
        hiragana = std::move(*p);
        data.erase(p);
    } else {
        hiragana = MeCabWord(raw);
    }
    data.insert(data.begin() + (data.empty() ? 0 : 1), std::move(hiragana));
    index                            = 0;
    is_initialized_with_dictionaries = true;
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
bool CandidateList::match(Candidates* const cmp) const noexcept {
    return data == cmp;
}
const fcitx::Text& CandidateList::label(int idx) const {
    static fcitx::Text label("");
    return label;
}
const CandidateWord& CandidateList::candidate(int idx) const {
    return *words[index_to_global(idx)];
}
int CandidateList::cursorIndex() const {
    return index_to_local(data->get_index());
}
int CandidateList::size() const {
    auto size   = data->get_data_size();
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
    data->move_index(-add);
}
void CandidateList::next() {
    size_t add  = page_size;
    auto   size = data->get_data_size();
    if(cursorIndex() + add >= static_cast<size_t>(size)) {
        add = size - 1 - cursorIndex();
    }
    data->move_index(page_size);
}
bool CandidateList::usedNextBefore() const {
    return true;
}
CandidateWord::~CandidateWord() {
}
int CandidateList::totalPages() const {
    auto size = data->get_data_size();
    return size / page_size + 1;
}
int CandidateList::currentPage() const {
    return data->get_index() / page_size;
}
void CandidateList::setPage(int page) {
    data->set_index(page * page_size);
}
fcitx::CandidateLayoutHint CandidateList::layoutHint() const {
    return fcitx::CandidateLayoutHint::NotSet;
}
void CandidateList::prevCandidate() {
    data->move_index(-1);
}
void CandidateList::nextCandidate() {
    data->move_index(1);
}
CandidateList::CandidateList(Candidates* const data, const size_t page_size) : data(data), page_size(page_size) {
    setPageable(this);
    setCursorMovable(this);
    for(const auto& w : data->get_labels()) {
        words.emplace_back(std::make_unique<CandidateWord>(fcitx::Text(w)));
    }
}
