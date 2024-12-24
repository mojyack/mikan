#pragma once
#include <algorithm>

#include <fcitx/candidatelist.h>

#include "mecab-model.hpp"
#include "mecab-word.hpp"
#include "misc.hpp"

namespace mikan {
class Candidates {
  protected:
    uint64_t index; // TODO: size_t

  public:
    auto move_index(const int val) -> void {
        index = (index + val) % get_data_size();
    }

    auto set_index(const uint64_t val) -> void {
        index = val % get_data_size();
    }

    auto get_index() const -> uint64_t {
        return index;
    }

    virtual auto get_data_size() const -> size_t                = 0;
    virtual auto get_labels() const -> std::vector<std::string> = 0;

    Candidates()          = default;
    virtual ~Candidates() = default;
};

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

    WordCandidates(const std::vector<MeCabModel*>& dictionaries, const WordCandidates& source) {
        const auto predicate_same_feature = [](const MeCabWord& elm, const MeCabWord& o) -> bool {
            return elm.get_feature() == o.get_feature();
        };

        const auto& raw     = source.get_raw().get_feature();
        const auto& current = source.get_current().get_feature();
        for(auto d : dictionaries) {
            auto& lattice = *d->lattice;
            lattice.set_request_type(MECAB_NBEST);
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

        static const auto find_feature = [](std::string_view feature) {
            return [feature](const MeCabWord& word) {
                return word.get_feature() == feature;
            };
        };

        // first candidate is current feature
        if(const auto p = std::ranges::find_if(data, find_feature(current)); p == data.end()) {
            data.insert(data.begin(), MeCabWord(current));
        } else if(p != data.begin()) {
            std::swap(*p, data[0]);
        }

        // second candidate is hiragana(raw)
        if(const auto p = std::ranges::find_if(data, find_feature(raw)); p == data.end()) {
            data.insert(data.begin() + 1, MeCabWord(raw));
        } else if(p != data.begin() + 1) {
            std::swap(*p, data[1]);
        }
        index                            = 0;
        is_initialized_with_dictionaries = true;
    }
};
} // namespace mikan
