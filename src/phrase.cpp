#include <mecab.h>

#include "phrase.hpp"
#include "candidate.hpp"
#include "type.hpp"

namespace {
std::string get_node_surface(const MeCab::Node* node) {
    return std::string(node->surface, node->length);
}
MeCabWord node_to_word(const MeCab::Node* node) {
    if(node->stat == MECAB_UNK_NODE) {
        return MeCabWord(get_node_surface(node));
    } else {
        return MeCabWord(node->feature, node->rcAttr, node->lcAttr, node->wcost, node->cost, true);
    }
}
}

ProtectionLevel Phrase::get_protection_level() const noexcept {
    return protection;
}
void Phrase::set_protection_level(ProtectionLevel level) noexcept {
    protection = level;
}
const MeCabWord& Phrase::get_raw() const {
    return candidates.get_raw();
}
const MeCabWord& Phrase::get_translated() const {
    return candidates.get_translated();
}
std::string& Phrase::get_mutable() {
    return candidates.get_mutable();
}
void Phrase::override_translated(const MeCabWord& translated) {
    candidates.override_translated(translated);
}
void Phrase::override_translated(MeCabWord&& translated) {
    candidates.override_translated(translated);
}
bool Phrase::is_candidates_initialized() const noexcept {
    return candidates.is_initialized();
}
bool Phrase::has_candidates() const noexcept {
    return candidates.has_candidates();
}
PhraseCandidates& Phrase::get_candidates() noexcept {
    return candidates;
}
void Phrase::reset_candidates(PhraseCandidates&& new_candidates) {
    candidates = std::move(new_candidates); 
}
Phrase::Phrase() : candidates(MeCabWord()) {};
Phrase::Phrase(const std::string& raw) : candidates(MeCabWord(raw)){
}
Phrase::Phrase(const MeCab::Node* node) : 
    candidates(MeCabWord(get_node_surface(node)), node_to_word(node)) {
}

bool SentenceCandidates::empty() const noexcept {
    return data.empty();
}
size_t SentenceCandidates::get_data_size() const noexcept {
    return data.size();
}
std::vector<std::string> SentenceCandidates::get_labels() const noexcept {
    std::vector<std::string> labels;
    for(auto& s : data) {
        labels.emplace_back();
        auto& label = labels.back();
        for(auto& p : s) {
            label += p.get_translated().get_feature();
        }
    }
    return labels;
}
void SentenceCandidates::reset(Sentences&& n) {
    data = std::move(n);
    index = 0;
}
Phrases* SentenceCandidates::get_current_ptr() {
    if(data.empty()) {
        return nullptr;
    }
    return &data[index];
}
Phrases& SentenceCandidates::operator[](size_t index) {
    return data[index];
}
void SentenceCandidates::clear() {
    data.clear();
}
SentenceCandidates::SentenceCandidates() {
}
