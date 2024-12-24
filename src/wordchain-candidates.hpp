#pragma once
#include "word.hpp"

namespace mikan {
class WordChainCandidates : public Candidates {
  private:
    WordChains data;

  public:
    auto empty() const -> bool {
        return data.empty();
    }

    auto get_data_size() const -> size_t {
        return data.size();
    }

    auto get_labels() const -> std::vector<std::string> {
        auto labels = std::vector<std::string>();
        for(auto& s : data) {
            auto& label = labels.emplace_back();
            for(auto& p : s) {
                label += p.get_translated().get_feature();
            }
        }
        return labels;
    }

    auto reset(WordChains&& n) -> void { // FIXME: as value
        data  = std::move(n);
        index = 0;
    }

    auto clear() -> void {
        data.clear();
    }

    auto operator[](const size_t index) -> WordChain& {
        return data[index];
    }

    WordChainCandidates() = default;
};
} // namespace mikan
