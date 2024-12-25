#include "word.hpp"
#include "misc.hpp"

namespace mikan {
auto Word::get_data_size() const -> size_t {
    if(candidates.size() > 2) {
        return candidates.size() - 2;
    } else {
        return 1;
    }
}

auto Word::get_labels() const -> std::vector<std::string> {
    if(candidates.size() > 2) {
        auto labels = std::vector<std::string>();
        for(auto i = candidates.begin() + 2; i < candidates.end(); i += 1) {
            labels.emplace_back(*i);
        }
        return labels;
    } else {
        return {feature()};
    }
}

auto Word::has_candidates() const -> bool {
    return candidates.size() > 2;
}

auto Word::feature() -> std::string& {
    if(candidates.size() > 2) {
        return candidates[index + 2];
    } else if(candidates.size() > 1) {
        return candidates[1];
    } else {
        return candidates[0];
    }
}

auto Word::raw() -> std::string& {
    return candidates[0];
}
auto Word::raw() const -> const std::string& {
    return candidates[0];
}

auto Word::feature() const -> const std::string& {
    return ((Word*)this)->feature();
}

auto Word::from_node(const MeCab::Node& node) -> Word {
    auto word = Word();
    word.candidates.emplace_back(std::string(node.surface, node.length));
    if(node.stat != MECAB_UNK_NODE) {
        word.candidates.emplace_back(node.feature);
    }
    return word;
}

auto Word::from_raw(std::string raw) -> Word {
    auto word       = Word();
    word.candidates = {std::move(raw)};
    return word;
}

auto Word::from_dictionaries(std::span<MeCabModel*> dicts, const Word& source) -> Word {
    auto ret = Word();
    ret.candidates.emplace_back(source.candidates[0]);
    if(source.candidates.size() > 1) {
        ret.candidates.emplace_back(source.candidates[1]);
    }

    const auto& raw = source.candidates[0];
    for(const auto dict : dicts) {
        auto& lattice = *dict->lattice;
        lattice.set_request_type(MECAB_NBEST);
        lattice.set_sentence(raw.data());
        lattice.set_feature_constraint(0, raw.size(), "*");
        dict->tagger->parse(&lattice);
        do {
            for(const auto* node = lattice.bos_node(); node; node = node->next) {
                if(node->stat != MECAB_NOR_NODE) {
                    continue;
                }
                emplace_unique(ret.candidates, std::string(node->feature));
            }
        } while(lattice.next());
        lattice.clear();
    }

    return ret;
}
} // namespace mikan
