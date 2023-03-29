#pragma once
#include <array>
#include <filesystem>
#include <fstream>
#include <thread>

#include "misc.hpp"
#include "phrase.hpp"
#include "share.hpp"
#include "tmp.hpp"
#include "util.hpp"

namespace mikan::engine {
struct History {
    std::string raw;
    MeCabWord   translation;
};

struct FeatureConstriant {
    size_t        begin;
    size_t        end;
    const Phrase* phrase;
};

inline auto split_strip(const std::string_view str, const char* const delm = " ") -> std::vector<std::string_view> {
    auto vec = split(str, delm);
    std::erase(vec, "");
    return vec;
}

static auto parse_line_to_history(const std::string_view line) -> std::optional<History> {
    const auto elms = split_strip(line, ",");
    auto       hist = History();
    if(elms.size() == 2) {
        hist.raw         = elms[0];
        hist.translation = MeCabWord(elms[1]);
    } else if(elms.size() == 5) {
        hist.raw = elms[0];

        using Attr = unsigned short;
        auto rattr = Attr();
        auto lattr = Attr();
        auto cost  = short();
        try {
            lattr = std::stoi(std::string(elms[1]));
            rattr = std::stoi(std::string(elms[2]));
            cost  = std::stoi(std::string(elms[3]));
        } catch(const std::invalid_argument&) {
            return std::nullopt;
        }
        hist.translation = MeCabWord(elms[4], rattr, lattr, cost, 0, true);
    } else {
        return std::nullopt;
    }
    return hist;
}

static auto parse_line_to_key_value(const std::string_view line) -> std::pair<std::string_view, std::string_view> {
    const auto vec = split_strip(line);
    if(vec.size() != 2) {
        return {};
    }
    return {vec[0], vec[1]};
}

static auto build_raw_and_constraints(const Phrases& source, const bool ignore_protection) -> std::pair<std::string, std::vector<FeatureConstriant>> {
    auto feature_constriants = std::vector<FeatureConstriant>();
    auto buffer              = std::string();
    for(auto p = source.begin(); p != source.end(); p += 1) {
        const auto& raw = p->get_raw().get_feature();
        if(!ignore_protection && p->get_protection_level() != ProtectionLevel::None) {
            auto begin = buffer.size();
            auto end   = begin + raw.size();
            feature_constriants.emplace_back(FeatureConstriant{begin, end, &*p});
        };
        buffer += raw;
    }
    return std::make_pair(buffer, feature_constriants);
}

static auto load_text_dictionary(const std::string_view path) -> std::vector<History> {
    auto res = std::vector<History>();
    if(!std::filesystem::is_regular_file(path)) {
        return res;
    }
    auto source = std::fstream(path);
    auto l      = std::string();
    while(std::getline(source, l)) {
        const auto hist = parse_line_to_history(l);
        if(!hist.has_value()) {
            continue;
        }
        res.emplace_back(hist.value());
    }
    return res;
}

static auto set_constraints(MeCab::Lattice& lattice, const std::vector<FeatureConstriant>& constraints) -> void {
    for(const auto& f : constraints) {
        auto& phrase = *f.phrase;
        lattice.set_feature_constraint(f.begin,
                                       f.end,
                                       phrase.get_protection_level() == ProtectionLevel::PreserveTranslation ? phrase.get_translated().get_feature().data() : "*");
    }
}

static auto parse_nodes(MeCab::Lattice& lattice, const bool needs_parsed_feature) -> std::pair<std::string, Phrases> {
    auto parsed         = Phrases();
    auto parsed_feature = std::string();
    for(const auto* node = lattice.bos_node(); node; node = node->next) {
        if(node->stat == MECAB_BOS_NODE || node->stat == MECAB_EOS_NODE) {
            continue;
        }
        parsed.emplace_back(node);
        if(needs_parsed_feature) {
            parsed_feature += node->feature;
        }
    }
    return std::make_pair(parsed_feature, parsed);
}

class Engine {
  private:
    Share&                   share;
    std::string              system_dictionary_path;
    std::string              history_file_path;
    std::string              dictionary_compiler_path;
    std::vector<std::string> user_dictionary_paths;
    std::thread              dictionary_updater;
    Event                    dictionary_update_event;
    std::vector<History>     histories;
    Critical<Sentences>      fix_requests;
    std::string              tmpdir;
    bool                     finish_dictionary_updater = false;
    bool                     enable_history            = true;

    auto load_configuration() -> bool {
        const auto user_config_dir = get_user_config_dir();
        if(!std::filesystem::is_directory(user_config_dir) && !std::filesystem::create_directories(user_config_dir)) {
            return false;
        }
        auto config = std::fstream(user_config_dir + "/mikan.conf");
        auto line   = std::string();
        while(std::getline(config, line)) {
            if(line.empty() || line[0] == '#') {
                continue;
            }

            auto [key, value] = parse_line_to_key_value(line);
            if(key.empty()) {
                continue;
            }

            auto error = false;
            if(key == "candidate_page_size") {
                try {
                    share.candidate_page_size = std::stoi(std::string(value));
                } catch(const std::invalid_argument&) {
                    error = true;
                }
            } else if(key == "auto_commit_threshold") {
                try {
                    share.auto_commit_threshold = std::stoi(std::string(value));
                } catch(const std::invalid_argument&) {
                    error = true;
                }
            } else if(key == "dictionary") {
                emplace_unique(user_dictionary_paths, user_config_dir + "/" + std::string(value));
            } else if(key == "insert_space") {
                if(value == "on") {
                    share.insert_space = InsertSpaceOptions::On;
                } else if(value == "off") {
                    share.insert_space = InsertSpaceOptions::Off;
                } else if(value == "smart") {
                    share.insert_space = InsertSpaceOptions::Smart;
                } else {
                    error = true;
                }
            } else if(key == "history") {
                if(value == "on") {
                    enable_history = true;
                } else if(value == "off") {
                    enable_history = false;
                } else {
                    error = true;
                }
            } else if(key == "dictionaries") {
                share.dictionary_path = value;
            } else {
                error = true;
            }
            if(error) {
                FCITX_WARN() << "error while parsing configuration: " << line;
            }
        }
        return true;
    }

    auto add_history(const History& word) -> bool {
        auto new_word         = true;
        auto word_not_changed = true;
        auto duplicates       = size_t(0);
        for(auto& l : histories) {
            if(l.raw != word.raw || !l.translation.has_same_attr(word.translation)) {
                continue;
            }
            if(l.translation.get_feature() == word.translation.get_feature()) {
                if(l.translation.get_word_cost() != word.translation.get_word_cost()) {
                    l.translation.override_word_cost(word.translation.get_word_cost());
                    word_not_changed = false;
                }
                new_word = false;
                break;
            } else {
                duplicates += 1;
            }
        }

        if(new_word) {
            histories.emplace_back(word);
            if(duplicates != 0) {
                MeCabWord& w = histories.back().translation;
                w.override_word_cost(w.get_word_cost() - duplicates);
            }
        }
        return new_word || !word_not_changed;
    }

    auto save_hisotry() const -> void {
        auto file = std::ofstream(history_file_path);
        for(const auto& l : histories) {
            auto line = std::string();
            line += l.raw + ",";
            line += std::to_string(l.translation.get_cattr_left()) + ",";
            line += std::to_string(l.translation.get_cattr_right()) + ",";
            line += std::to_string(l.translation.get_word_cost()) + ",";
            line += l.translation.get_feature();
            file << line << std::endl;
        }
    }

    auto dump_internal_dict(const std::string_view path) const -> void {
        auto to_compile = std::vector<History>();
        for(const auto& d : user_dictionary_paths) {
            auto hists = load_text_dictionary(d.data());
            std::copy(hists.begin(), hists.end(), std::back_inserter(to_compile));
        }
        std::copy(histories.begin(), histories.end(), std::back_inserter(to_compile));
        auto file = std::ofstream(path);
        for(const auto& l : to_compile) {
            const auto& t     = l.translation;
            const auto  lattr = t.has_valid_word_info() ? t.get_cattr_left() : 0;
            const auto  rattr = t.has_valid_word_info() ? t.get_cattr_right() : 0;
            const auto  cost  = t.has_valid_word_info() ? t.get_word_cost() : 0;
            file << l.raw << "," << std::to_string(lattr) << "," << std::to_string(rattr) << "," << std::to_string(cost) << "," << t.get_feature() << std::endl;
        }
    }

    auto compile_and_reload_user_dictionary() -> void {
        auto       tmp = TemporaryDirectory();
        const auto csv = tmp.str() + "/dict.csv";
        const auto bin = tmp.str() + "/dict.bin";
        dump_internal_dict(csv);

        auto command = std::string();
        command += dictionary_compiler_path;
        command += " -d ";
        command += system_dictionary_path;
        command += " -u ";
        command += bin;
        command += " -f utf-8 -t utf-8 ";
        command += csv;
        command += " >& /dev/null";
        [[maybe_unused]] auto code = system(command.data());

        reload_dictionary(bin.data());
    }

    auto reload_dictionary(const char* const user_dict = nullptr) -> void {
        share.primary_vocabulary = std::make_shared<MeCabModel>(system_dictionary_path.data(), user_dict, true);
    }

    auto recalc_cost(const Phrases& source) const -> long {
        // source.back().get_translated().get_total_cost() sould be total cost.
        // but it seems that mecab sets incorrect cost values when parsing delimiter positions and words at the same time.
        // so we have to specify constraints to all words before parsing to get the correct cost.
        auto copy = source;
        for(auto& p : copy) {
            p.set_protection_level(ProtectionLevel::PreserveTranslation);
        }
        auto       source_data = build_raw_and_constraints(source, false);
        const auto buffer      = std::move(source_data.first);
        const auto constraints = std::move(source_data.second);

        auto  dic     = share.primary_vocabulary;
        auto& lattice = *dic->lattice;
        lattice.set_request_type(MECAB_ONE_BEST);
        lattice.set_sentence(buffer.data());
        set_constraints(lattice, constraints);
        dic->tagger->parse(&lattice);
        return parse_nodes(lattice, true).second.back().get_translated().get_total_cost();
    }

    auto compare_and_fix_dictionary(const Phrases& wants) -> void {
        {
            // before comparing, add words which is unknown and not from system dictionary.
            auto added = false;
            for(const auto& p : wants) {
                auto& word = p.get_translated();
                if(word.has_valid_word_info()) {
                    continue;
                }
                added = true;
                add_history({word.get_feature(), word});
            }
            if(added) {
                compile_and_reload_user_dictionary();
            }
        }

        const auto source_cost = recalc_cost(wants);
        while(1) {
            auto to_decrements      = std::vector<History>();
            auto best               = translate_phrases(wants, true, true)[0];
            auto all_phrases_passed = true;
            for(auto& p : best) {
                p.set_protection_level(ProtectionLevel::None);
            }
            const auto total_diff = source_cost - recalc_cost(best);
            for(auto i = size_t(0); i < wants.size(); i += 1) {
                auto&         pb = best[i];
                const Phrase& pw = wants[i];
                if(pb.get_raw().get_feature() == pw.get_raw().get_feature() && pb.get_translated().get_feature() == pw.get_translated().get_feature()) {
                    continue;
                }
                all_phrases_passed = false;
                auto left          = std::string();
                for(auto p = size_t(i); p < best.size(); p += 1) {
                    left += best[p].get_raw().get_feature();
                }
                auto& sw = pw.get_raw().get_feature();
                to_decrements.emplace_back(History{sw, pw.get_translated()});

                pb.get_mutable_feature() = left.substr(0, sw.size());
                pb.override_translated(pw.get_translated());
                pb.set_protection_level(ProtectionLevel::PreserveTranslation);
                if(i + 1 < wants.size()) {
                    best.resize(i + 2);
                    best[i + 1].get_mutable_feature() = left.substr(sw.size());
                } else {
                    best.resize(i + 1);
                }
                best = translate_phrases(best, true, false)[0];
            }
            if(all_phrases_passed) {
                return;
            }
            const auto cost_decrement = total_diff / to_decrements.size() + 1;
            for(auto& d : to_decrements) {
                auto&      word         = d.translation;
                const auto found        = std::find_if(histories.begin(), histories.end(), [&d](const History& o) {
                    return d.raw == o.raw && d.translation.get_feature() == o.translation.get_feature() && d.translation.has_same_attr(o.translation);
                });
                const auto current_cost = (found != histories.end() ? found->translation : word).get_word_cost();
                word.override_word_cost(current_cost - cost_decrement);
                add_history(d);
            }
            compile_and_reload_user_dictionary();
        }
    }

    auto dictionary_updater_main() -> void {
    loop:
        if(finish_dictionary_updater) {
            return;
        }
        auto request = std::optional<Phrases>();
        {
            auto [lock, requests] = fix_requests.access();
            if(!requests.empty()) {
                request = std::move(requests[0]);
                requests.erase(requests.begin());
            }
        }
        if(request) {
            compare_and_fix_dictionary(*request);
        } else {
            dictionary_update_event.wait();
        }
        goto loop;
    }

  public:
    auto translate_phrases(const Phrases& source, const bool best_only, const bool ignore_protection = false) const -> Sentences {
        constexpr auto N_BEST_LIMIT = size_t(30);

        auto       result      = Sentences();
        auto       source_data = build_raw_and_constraints(source, ignore_protection);
        const auto buffer      = std::move(source_data.first);
        const auto constraints = std::move(source_data.second);
        {
            auto  dic     = share.primary_vocabulary;
            auto& lattice = *dic->lattice;
            lattice.set_request_type(best_only ? MECAB_ONE_BEST : MECAB_NBEST);
            lattice.set_sentence(buffer.data());
            set_constraints(lattice, constraints);
            dic->tagger->parse(&lattice);
            auto result_features = std::vector<std::string>();
            while(1) {
                const auto parsed_nodes   = parse_nodes(lattice, true);
                const auto parsed_feature = std::string(std::move(parsed_nodes.first));
                const auto parsed         = Phrases(std::move(parsed_nodes.second));
                auto       matched        = false;
                for(const auto& r : result_features) {
                    if(r == parsed_feature) {
                        matched = true;
                        break;
                    }
                }
                if(!matched) {
                    result.emplace_back(parsed);
                    result_features.emplace_back(std::move(parsed_feature));
                }
                if(!lattice.next() || result.size() >= N_BEST_LIMIT) {
                    break;
                }
            }
            lattice.clear();
        }
        if(ignore_protection) {
            return result;
        }

        // retrive protected phrases.
        for(auto& r : result) {
            auto total_bytes          = size_t(0);
            auto protected_phrase_pos = constraints.cbegin();
            for(auto& p : r) {
                if(protected_phrase_pos == constraints.cend()) {
                    break;
                }
                if(total_bytes == protected_phrase_pos->begin) {
                    // this is a phrase from a protected one.
                    p.set_protection_level(protected_phrase_pos->phrase->get_protection_level());
                    if(p.get_protection_level() == ProtectionLevel::PreserveTranslation) {
                        p = *protected_phrase_pos->phrase;
                    }
                    protected_phrase_pos += 1;
                } else if(total_bytes > protected_phrase_pos->begin) {
                    panic("protected phrase lost");
                }
                total_bytes += p.get_raw().get_feature().size();
            }
        }

        return result;
    }

    auto request_fix_dictionary(Phrases wants) -> void {
        if(!enable_history) {
            return;
        }

        auto [lock, requests] = fix_requests.access();
        requests.emplace_back(wants);
        dictionary_update_event.wakeup();
    }

    Engine(Share& share) : share(share) {
        if(!load_configuration()) {
            panic("failed to load configuration.");
        }
        for(const auto& entry : std::filesystem::directory_iterator(share.dictionary_path)) {
            if(entry.path().filename() == "system") {
                system_dictionary_path = entry.path().string();
            } else {
                auto new_dic = std::unique_ptr<MeCabModel>();
                try {
                    new_dic.reset(new MeCabModel(entry.path().string().data(), nullptr, false));
                } catch(const std::runtime_error&) {
                    FCITX_WARN() << "failed to load addtional dictionary. " << entry.path();
                    continue;
                }
                share.additional_vocabularies.push_back(std::move(new_dic));
            }
        }
        dynamic_assert(!system_dictionary_path.empty(), "failed to find system dictionary");

        if(enable_history) {
            const auto cache_dir = get_user_cache_dir();
            if(!std::filesystem::is_directory(cache_dir)) {
                std::filesystem::create_directories(cache_dir);
            }
            history_file_path = cache_dir + "/history.csv";

            histories          = load_text_dictionary(history_file_path.data());
            dictionary_updater = std::thread(std::bind(&Engine::dictionary_updater_main, this));
        }

        if(const auto compiler_path = get_dictionary_compiler_path()) {
            dictionary_compiler_path = compiler_path.value() + "/mecab-dict-index";
            compile_and_reload_user_dictionary();
        } else {
            FCITX_WARN() << "missing dictionary compiler";
            enable_history = false;
            reload_dictionary();
        }

        share.key_config.keys.resize(static_cast<size_t>(Actions::ActionsLimit));
        share.key_config[Actions::Backspace]          = {{FcitxKey_BackSpace}};
        share.key_config[Actions::ReinterpretNext]    = {{FcitxKey_space}};
        share.key_config[Actions::ReinterpretPrev]    = {{FcitxKey_space, fcitx::KeyState::Shift}};
        share.key_config[Actions::CandidateNext]      = {{FcitxKey_Down}, {FcitxKey_J, fcitx::KeyState::Ctrl}};
        share.key_config[Actions::CandidatePrev]      = {{FcitxKey_Up}, {FcitxKey_K, fcitx::KeyState::Ctrl}};
        share.key_config[Actions::CandidatePageNext]  = {{FcitxKey_Left}};
        share.key_config[Actions::CandidatePagePrev]  = {{FcitxKey_Right}};
        share.key_config[Actions::Commit]             = {{FcitxKey_Return}};
        share.key_config[Actions::PhraseNext]         = {{FcitxKey_Left}, {FcitxKey_H, fcitx::KeyState::Ctrl}};
        share.key_config[Actions::PhrasePrev]         = {{FcitxKey_Right}, {FcitxKey_L, fcitx::KeyState::Ctrl}};
        share.key_config[Actions::SplitPhraseLeft]    = {{FcitxKey_Left, fcitx::KeyState::Ctrl}, {FcitxKey_H, fcitx::KeyState::Alt}};
        share.key_config[Actions::SplitPhraseRight]   = {{FcitxKey_Right, fcitx::KeyState::Ctrl}, {FcitxKey_L, fcitx::KeyState::Alt}};
        share.key_config[Actions::MergePhraseLeft]    = {{FcitxKey_Left, fcitx::KeyState::Ctrl_Alt}, {FcitxKey_J, fcitx::KeyState::Alt}};
        share.key_config[Actions::MergePhraseRight]   = {{FcitxKey_Right, fcitx::KeyState::Ctrl_Alt}, {FcitxKey_K, fcitx::KeyState::Alt}};
        share.key_config[Actions::MoveSeparatorLeft]  = {{FcitxKey_Left, fcitx::KeyState::Alt}, {FcitxKey_H, fcitx::KeyState::Ctrl_Alt}};
        share.key_config[Actions::MoveSeparatorRight] = {{FcitxKey_Right, fcitx::KeyState::Alt}, {FcitxKey_L, fcitx::KeyState::Ctrl_Alt}};
        share.key_config[Actions::ConvertKatakana]    = {{FcitxKey_q}};
    }

    ~Engine() {
        if(enable_history) {
            finish_dictionary_updater = true;
            dictionary_update_event.wakeup();
            dictionary_updater.join();
            save_hisotry();
        }
    }
};
} // namespace mikan::engine
