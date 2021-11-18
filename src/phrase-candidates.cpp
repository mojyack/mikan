#include "candidate.hpp"
#include "mecab-model.hpp"
#include "misc.hpp"

namespace mikan {
auto PhraseCandidates::get_raw() const -> const MeCabWord& {
    if(data.size() < 2) {
        return get_translated();
    } else {
        return data[1];
    }
}
auto PhraseCandidates::get_translated() const -> const MeCabWord& {
    return data[index];
}
auto PhraseCandidates::get_mutable_feature() -> std::string& {
    return data[data.size() < 2 ? 0 : 1].get_mutable_feature();
}
auto PhraseCandidates::override_translated(const MeCabWord& translated) -> void {
    data[0] = translated;
}
auto PhraseCandidates::override_translated(MeCabWord&& translated) -> void {
    data[0] = std::move(translated);
}
auto PhraseCandidates::is_initialized() const -> bool {
    return is_initialized_with_dictionaries;
}
auto PhraseCandidates::has_candidates() const -> bool {
    return data.size() > 1;
}
auto PhraseCandidates::get_data_size() const -> size_t {
    return data.size();
}
auto PhraseCandidates::get_data_ref() const -> const MeCabWords& {
    return data;
}
auto PhraseCandidates::get_current() const -> const MeCabWord& {
    return data[index];
}
auto PhraseCandidates::get_labels() const -> std::vector<std::string> {
    auto labels = std::vector<std::string>();
    for(auto& d : data) {
        labels.emplace_back(d.get_feature());
    }
    return labels;
}
auto PhraseCandidates::operator[](size_t index) const -> const MeCabWord& {
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
PhraseCandidates::PhraseCandidates(const std::vector<MeCabModel*>& dictionaries, const PhraseCandidates& source) {
    const auto predicate_same_feature = [](const MeCabWord& elm, const MeCabWord& o) -> bool {
        return elm.get_feature() == o.get_feature();
    };

    const auto& raw = source.get_raw().get_feature();
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

    // first candidate is current feature
    auto& current = source.get_current().get_feature();
    if(auto p = std::find_if(data.begin(), data.end(), [&current](const MeCabWord& o) {
           return o.get_feature() == current;
       });
       p == data.end()) {
        data.insert(data.begin(), MeCabWord(current));
    } else if(p != data.begin()) {
        std::swap(*p, data[0]);
    }

    // second candidate is hiragana
    if(current != raw) {
        if(auto p = std::find_if(data.begin(), data.end(), [&raw](const MeCabWord& o) {
               return o.get_feature() == raw;
           });
           p == data.end()) {
            data.insert(data.begin() + 1, MeCabWord(raw));
        } else if(p != data.begin() + 1) {
            std::swap(*p, data[1]);
        }
    }
    index                            = 0;
    is_initialized_with_dictionaries = true;
}
} // namespace mikan
