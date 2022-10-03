#pragma once
#include <fcitx/candidatelist.h>

#include "mecab-model.hpp"
#include "mecab-word.hpp"
#include "misc.hpp"

namespace mikan {
class Candidates {
  protected:
    uint64_t index;

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

class PhraseCandidates : public Candidates {
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

    PhraseCandidates() = default;

    PhraseCandidates(MeCabWord raw) {
        data  = MeCabWords{std::move(raw)};
        index = 0;
    }

    PhraseCandidates(MeCabWord raw, MeCabWord translated) {
        data  = MeCabWords{std::move(translated), std::move(raw)};
        index = 0;
    }

    PhraseCandidates(const std::vector<MeCabModel*>& dictionaries, const PhraseCandidates& source) {
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
};

class CandidateWord : public fcitx::CandidateWord {
  public:
    auto select(fcitx::InputContext* inputContext) const -> void override {}
    CandidateWord(fcitx::Text text) : fcitx::CandidateWord(std::move(text)){};
    ~CandidateWord() = default;
};

class CandidateList : public fcitx::CandidateList,
                      public fcitx::PageableCandidateList,
                      public fcitx::CursorMovableCandidateList {
  private:
    Candidates* const                           data;
    const size_t                                page_size;
    std::vector<std::unique_ptr<CandidateWord>> words;

    auto index_to_local(const size_t idx) const -> size_t {
        return idx - currentPage() * page_size;
    }

    auto index_to_global(const size_t idx) const -> size_t {
        return idx + currentPage() * page_size;
    }

  public:
    // CandidateList
    auto label(const int idx) const -> const fcitx::Text& override {
        const static auto label = fcitx::Text("");
        return label;
    }

    auto candidate(const int idx) const -> const CandidateWord& override {
        return *words[index_to_global(idx)];
    }

    auto cursorIndex() const -> int override {
        return index_to_local(data->get_index());
    }

    auto size() const -> int override {
        const auto size   = data->get_data_size();
        const auto remain = size - currentPage() * page_size;
        return remain > page_size ? page_size : remain;
    }

    // PageableCandidateList
    auto hasPrev() const -> bool override {
        return currentPage() >= 1;
    }

    auto hasNext() const -> bool override {
        return currentPage() + 1 < totalPages();
    }

    auto prev() -> void override {
        auto add = page_size;
        if(add > static_cast<size_t>(cursorIndex())) {
            add = cursorIndex();
        }
        data->move_index(-add);
    }

    auto next() -> void override {
        auto       add  = page_size;
        const auto size = data->get_data_size();
        if(cursorIndex() + add >= static_cast<size_t>(size)) {
            add = size - 1 - cursorIndex();
        }
        data->move_index(page_size);
    }

    auto usedNextBefore() const -> bool override {
        return true;
    }

    auto totalPages() const -> int override {
        const auto size = data->get_data_size();
        return size / page_size + 1;
    }

    auto currentPage() const -> int override {
        return data->get_index() / page_size;
    }

    auto setPage(int page) -> void override {
        data->set_index(page * page_size);
    }

    auto layoutHint() const -> fcitx::CandidateLayoutHint override {
        return fcitx::CandidateLayoutHint::NotSet;
    }

    // CursorMovableCandidateList
    auto prevCandidate() -> void override {
        data->move_index(-1);
    }

    auto nextCandidate() -> void override {
        data->move_index(1);
    }

    CandidateList(Candidates* const data, size_t page_size) : data(data), page_size(page_size) {
        setPageable(this);
        setCursorMovable(this);
        for(const auto& w : data->get_labels()) {
            words.emplace_back(std::make_unique<CandidateWord>(fcitx::Text(w)));
        }
    }
};
} // namespace mikan
