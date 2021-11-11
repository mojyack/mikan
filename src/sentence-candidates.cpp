#include "phrase.hpp"

namespace mikan {
auto SentenceCandidates::empty() const -> bool {
    return data.empty();
}
auto SentenceCandidates::get_data_size() const -> size_t {
    return data.size();
}
auto SentenceCandidates::get_labels() const -> std::vector<std::string> {
    auto labels = std::vector<std::string>();
    for(auto& s : data) {
        labels.emplace_back();
        auto& label = labels.back();
        for(auto& p : s) {
            label += p.get_translated().get_feature();
        }
    }
    return labels;
}
auto SentenceCandidates::reset(Sentences&& n) -> void {
    data  = std::move(n);
    index = 0;
}
auto SentenceCandidates::get_current_ptr() -> Phrases* {
    if(data.empty()) {
        return nullptr;
    }
    return &data[index];
}
auto SentenceCandidates::operator[](const size_t index) -> Phrases& {
    return data[index];
}
auto SentenceCandidates::clear() -> void {
    data.clear();
}
} // namespace mikan
