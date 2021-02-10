#include "phrase.hpp"

void SentenceCandidates::clear_data() {
    data.clear();
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
            label += p.translation;
        }
    }
    return labels;
}
void SentenceCandidates::reset(Sentences&& n) {
    data = std::move(n);
    if(data.empty()) {
        index.reset();
    } else {
        index = 0;
    }
}
Phrases* SentenceCandidates::get_current_ptr() {
    if(!index) {
        return nullptr;
    }
    return &data[index.value()];
}
void SentenceCandidates::clear() {
    index.reset();
    clear_data();
}
SentenceCandidates::SentenceCandidates() {
    initialized = true;
}
