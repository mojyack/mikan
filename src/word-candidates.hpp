#pragma once
#include "candidates.hpp"

#include "mecab-model.hpp"
#include "mecab-word.hpp"

namespace mikan {
class WordCandidates : public Candidates {
  private:
    MeCabWords data;
    bool       is_initialized_with_dictionaries = false;

  public:
    auto get_raw() const -> const MeCabWord& {
        if(data.size() < 2) {
            return get_translated();
        } else {
            return data[1];
        }
    }

    auto get_translated() const -> const MeCabWord& {
        return data[index];
    }

    auto get_mutable_feature() -> std::string& {
        return data[data.size() < 2 ? 0 : 1].get_mutable_feature();
    }

    auto override_translated(const MeCabWord& translated) -> void {
        data[0] = translated;
    }

    auto override_translated(MeCabWord&& translated) -> void {
        data[0] = std::move(translated);
    }

    auto is_initialized() const -> bool {
        return is_initialized_with_dictionaries;
    }

    auto has_candidates() const -> bool {
        return data.size() > 1;
    }

    auto get_data_size() const -> size_t {
        return data.size();
    }

    auto get_data_ref() const -> const MeCabWords& {
        return data;
    }

    auto get_current() const -> const MeCabWord& {
        return data[index];
    }

    auto get_labels() const -> std::vector<std::string> {
        auto labels = std::vector<std::string>();
        for(auto& d : data) {
            labels.emplace_back(d.get_feature());
        }
        return labels;
    }

    auto operator[](size_t index) const -> const MeCabWord& {
        return data[index];
    }

    WordCandidates() = default;

    WordCandidates(MeCabWord raw) {
        data  = MeCabWords{std::move(raw)};
        index = 0;
    }

    WordCandidates(MeCabWord raw, MeCabWord translated) {
        data  = MeCabWords{std::move(translated), std::move(raw)};
        index = 0;
    }

    WordCandidates(const std::vector<MeCabModel*>& dictionaries, const WordCandidates& source); // FIXME: use span
};
} // namespace mikan
