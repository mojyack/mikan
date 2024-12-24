#include <algorithm>

#include "misc.hpp"
#include "word-candidates.hpp"

namespace mikan {
WordCandidates::WordCandidates(const std::vector<MeCabModel*>& dictionaries, const WordCandidates& source) {
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
} // namespace mikan
