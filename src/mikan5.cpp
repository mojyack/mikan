#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <bits/stdint-uintn.h>
#include <fcitx-utils/key.h>
#include <fcitx-utils/keysym.h>
#include <fcitx-utils/keysymgen.h>
#include <fcitx-utils/log.h>
#include <fcitx-utils/textformatflags.h>
#include <fcitx-utils/utf8.h>
#include <fcitx/addonfactory.h>
#include <fcitx/addoninstance.h>
#include <fcitx/addonmanager.h>
#include <fcitx/event.h>
#include <fcitx/inputcontext.h>
#include <fcitx/inputmethodengine.h>
#include <fcitx/inputmethodentry.h>
#include <fcitx/inputpanel.h>
#include <fcitx/instance.h>
#include <fcitx/text.h>
#include <mecab.h>

#include "candidate.hpp"
#include "config.h"
#include "configuration.hpp"
#include "mecab-model.hpp"
#include "mikan5.hpp"
#include "misc.hpp"
#include "phrase.hpp"
#include "romaji-table.hpp"
#include "state.hpp"
#include "type.hpp"

namespace {
std::optional<History> parse_line_to_history(const std::string& line) {
    size_t comma_index = 0;
    size_t commas[4];
    auto   pos = line.find(',');
    while(pos != std::string::npos) {
        if(comma_index >= 4) {
            comma_index = 0;
            break;
        }
        commas[comma_index] = pos;
        comma_index += 1;
        pos = line.find(',', pos + 1);
    }
    if(comma_index != 1 && comma_index != 4) {
        // error.
        return std::nullopt;
    }
    bool    simple = comma_index == 1;
    History hist;
    hist.raw = std::string(line.begin() + 0, line.begin() + commas[0]);
    if(simple) {
        hist.translation = MeCabWord(std::string(line.begin() + commas[0] + 1, line.end()));
    } else {
        unsigned short rattr, lattr;
        short          cost;
        try {
            lattr = stoi(std::string(line.begin() + commas[0] + 1, line.begin() + commas[1]));
            rattr = stoi(std::string(line.begin() + commas[1] + 1, line.begin() + commas[2]));
            cost  = stoi(std::string(line.begin() + commas[2] + 1, line.begin() + commas[3]));
        } catch(const std::invalid_argument&) {
            return std::nullopt;
            // error.
        }
        hist.translation = MeCabWord(std::string(line.begin() + commas[3] + 1, line.end()), rattr, lattr, cost, 0, true);
    }
    if(hist.raw.empty() || hist.translation.get_feature().empty()) {
        return std::nullopt;
    }
    return hist;
}
std::vector<History> load_histories(const char* path) {
    std::vector<History> res;
    if(!std::filesystem::is_regular_file(path)) {
        return res;
    }
    std::fstream source(path);
    std::string  l;
    while(std::getline(source, l)) {
        auto hist = parse_line_to_history(l);
        if(!hist.has_value()) {
            continue;
        }
        res.emplace_back(hist.value());
    }
    return res;
}
struct FeatureConstriant {
    size_t        begin;
    size_t        end;
    const Phrase* phrase;
};
std::pair<std::string, std::vector<FeatureConstriant>> build_raw_and_constraints(const Phrases& source, bool ignore_protection) {
    std::vector<FeatureConstriant> feature_constriants;
    std::string                    buffer;
    for(auto p = source.begin(); p != source.end(); ++p) {
        const std::string& raw = p->get_raw().get_feature();
        if(!ignore_protection && p->get_protection_level() != ProtectionLevel::NONE) {
            size_t begin = buffer.size();
            size_t end   = begin + raw.size();
            feature_constriants.emplace_back(FeatureConstriant{begin, end, &*p});
        };
        buffer += raw;
    }
    return std::make_pair(buffer, feature_constriants);
}
void set_constraints(MeCab::Lattice& lattice, const std::vector<FeatureConstriant>& constraints) {
    for(const auto& f : constraints) {
        auto& phrase = *f.phrase;
        lattice.set_feature_constraint(f.begin, f.end, phrase.get_protection_level() == ProtectionLevel::PRESERVE_TRANSLATION ? phrase.get_translated().get_feature().data() : "*");
    }
}
std::pair<std::string, Phrases> parse_nodes(MeCab::Lattice& lattice, bool needs_parsed_feature) {
    Phrases     parsed;
    std::string parsed_feature;
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
} // namespace
bool MikanEngine::load_configuration() {
    constexpr const char* CONFIG_FILE_NAME              = "mikan.conf";
    constexpr const char* CANDIDATE_PAGE_SIZE           = "candidate_page_size";
    constexpr const char* AUTO_COMMIT_THRESHOLD         = "auto_commit_threshold";
    constexpr const char* DICTIONARY                    = "dictionary";
    constexpr const char* INSERT_SPACE                  = "insert_space";
    constexpr size_t      DEFAULT_CANDIDATE_PAGE_SIZE   = 10;
    constexpr size_t      DEFAULT_AUTO_COMMIT_THRESHOLD = 8;
    auto                  user_config_dir               = get_user_config_dir();
    if(!std::filesystem::is_directory(user_config_dir) && !std::filesystem::create_directories(user_config_dir)) {
        return false;
    }
    share.candidate_page_size   = DEFAULT_CANDIDATE_PAGE_SIZE;
    share.auto_commit_threshold = DEFAULT_AUTO_COMMIT_THRESHOLD;
    {
        std::fstream config(user_config_dir + CONFIG_FILE_NAME);
        std::string  l;
        while(std::getline(config, l)) {
            std::string entry_name;
            std::string value;
            {
                auto space = l.find(' ');
                if(space == std::string::npos) {
                    continue;
                }
                entry_name = l.substr(0, space);
                for(auto i = space; l[i] != '\0'; ++i) {
                    if(space != ' ' && space != '\t') {
                        break;
                    }
                }
                value = l.substr(space + 1);
            }
            bool error = false;
            if(entry_name == CANDIDATE_PAGE_SIZE) {
                try {
                    share.candidate_page_size = std::stoi(value);
                } catch(const std::invalid_argument&) {
                    error = true;
                }
            } else if(entry_name == AUTO_COMMIT_THRESHOLD) {
                try {
                    share.auto_commit_threshold = std::stoi(value);
                } catch(const std::invalid_argument&) {
                    error = true;
                }
            } else if(entry_name == DICTIONARY) {
                emplace_unique(user_dictionary_paths, user_config_dir + value);
            } else if(entry_name == INSERT_SPACE) {
                auto option = value == "on" ? InsertSpaceOptions::On : value == "off" ? InsertSpaceOptions::Off : value == "smart" ? InsertSpaceOptions::Smart : InsertSpaceOptions::None;
                if(option != InsertSpaceOptions::None) {
                    share.insert_space = option;
                } else {
                    error = true;
                }
            }
            if(error) {
                FCITX_WARN() << "error while parsing configuration: " << entry_name;
            }
        }
    }
    return true;
}
bool MikanEngine::add_history(const History& word) {
    bool   new_word         = true;
    bool   word_not_changed = true;
    size_t duplicates       = 0;
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
void MikanEngine::save_hisotry() const {
    std::ofstream file(history_file_path);
    for(const auto& l : histories) {
        std::string line;
        line += l.raw + ",";
        line += std::to_string(l.translation.get_cattr_left()) + ",";
        line += std::to_string(l.translation.get_cattr_right()) + ",";
        line += std::to_string(l.translation.get_word_cost()) + ",";
        line += l.translation.get_feature();
        file << line << std::endl;
    }
}
bool MikanEngine::compile_user_dictionary() const {
    std::string csv = "/tmp/mikan-tmp";
    while(std::filesystem::exists(csv)) {
        csv += '.';
    }

    {
        std::vector<History> to_compile;
        for(const auto& d : user_dictionary_paths) {
            auto hists = load_histories(d.data());
            std::copy(hists.begin(), hists.end(), std::back_inserter(to_compile));
        }
        std::copy(histories.begin(), histories.end(), std::back_inserter(to_compile));
        std::ofstream file(csv);
        for(const auto& l : to_compile) {
            auto& t     = l.translation;
            short lattr = t.has_valid_word_info() ? t.get_cattr_left() : 0;
            short rattr = t.has_valid_word_info() ? t.get_cattr_right() : 0;
            short cost  = t.has_valid_word_info() ? t.get_word_cost() : 0;
            file << l.raw << "," << std::to_string(lattr) << "," << std::to_string(rattr) << "," << std::to_string(cost) << "," << t.get_feature() << std::endl;
        }
    }

    std::string command;
    command += dictionary_compiler_path;
    command += " -d ";
    command += system_dictionary_path;
    command += " -u ";
    command += history_dictionary_path;
    command += " -f utf-8 -t utf-8 ";
    command += csv;
    command += " >& /dev/null";
    auto result_code = WEXITSTATUS(system(command.data()));
    std::filesystem::remove(csv);
    return result_code == 0;
}
void MikanEngine::reload_dictionary() {
    auto new_dic = new MeCabModel(system_dictionary_path.data(), history_dictionary_path.data(), true);
    auto old_dic = share.primary_vocabulary.load();
    share.primary_vocabulary.store(new_dic);
    if(old_dic != nullptr) {
        delete old_dic;
    }
}
long MikanEngine::recalc_cost(const Phrases& source) const {
    // source.back().get_translated().get_total_cost() sould be total cost.
    // but it seems that mecab sets incorrect cost values when parsing delimiter positions and words at the same time.
    // so we have to specify constraints to all words before parsing to get the correct cost.
    auto copy = source;
    for(auto& p : copy) {
        p.set_protection_level(ProtectionLevel::PRESERVE_TRANSLATION);
    }
    auto       source_data = build_raw_and_constraints(source, false);
    const auto buffer      = std::move(source_data.first);
    const auto constraints = std::move(source_data.second);

    std::lock_guard<std::mutex> lock(share.primary_vocabulary.mutex);

    auto& dic     = share.primary_vocabulary.data;
    auto& lattice = *dic->lattice;
    lattice.set_request_type(MECAB_ONE_BEST);
    lattice.set_sentence(buffer.data());
    set_constraints(lattice, constraints);
    dic->tagger->parse(&lattice);
    return parse_nodes(lattice, true).second.back().get_translated().get_total_cost();
}
void MikanEngine::compare_and_fix_dictionary(const Phrases& wants) {
    {
        // before comparing, add words which is unknown and not from system dictionary.
        bool added = false;
        for(const auto& p : wants) {
            auto& word = p.get_translated();
            if(word.has_valid_word_info()) {
                continue;
            }
            added = true;
            add_history({word.get_feature(), word});
        }
        if(added) {
            compile_user_dictionary();
            reload_dictionary();
        }
    }

    const long source_cost = recalc_cost(wants);
    while(1) {
        Histories to_decrements;
        Phrases   best               = translate_phrases(wants, true, true)[0];
        bool      all_phrases_passed = true;
        for(auto& p : best) {
            p.set_protection_level(ProtectionLevel::NONE);
        }
        const long total_diff = source_cost - recalc_cost(best);
        for(size_t i = 0; i < wants.size(); ++i) {
            Phrase&       pb = best[i];
            const Phrase& pw = wants[i];
            if(pb.get_raw().get_feature() == pw.get_raw().get_feature() && pb.get_translated().get_feature() == pw.get_translated().get_feature()) {
                continue;
            }
            all_phrases_passed = false;
            std::string left;
            for(size_t p = i; p < best.size(); ++p) {
                left += best[p].get_raw().get_feature();
            }
            auto& sw = pw.get_raw().get_feature();
            to_decrements.emplace_back(History{sw, pw.get_translated()});

            pb.get_mutable() = left.substr(0, sw.size());
            pb.override_translated(pw.get_translated());
            pb.set_protection_level(ProtectionLevel::PRESERVE_TRANSLATION);
            if(i + 1 < wants.size()) {
                best.resize(i + 2);
                best[i + 1].get_mutable() = left.substr(sw.size());
            } else {
                best.resize(i + 1);
            }
            best = translate_phrases(best, true, false)[0];
        }
        if(all_phrases_passed) {
            return;
        }
        const long cost_decrement = total_diff / to_decrements.size() + 1;
        for(auto& d : to_decrements) {
            auto& word         = d.translation;
            auto  found        = std::find_if(histories.begin(), histories.end(), [&d](const History& o) {
                return d.raw == o.raw && d.translation.get_feature() == o.translation.get_feature() && d.translation.has_same_attr(o.translation);
            });
            long  current_cost = (found != histories.end() ? found->translation : word).get_word_cost();
            word.override_word_cost(current_cost - cost_decrement);
            add_history(d);
        }
        compile_user_dictionary();
        reload_dictionary();
   }
}
Sentences MikanEngine::translate_phrases(const Phrases& source, bool best_only, bool ignore_protection) const {
    constexpr size_t N_BEST_LIMIT = 30;

    Sentences  result;
    auto       source_data = build_raw_and_constraints(source, ignore_protection);
    const auto buffer      = std::move(source_data.first);
    const auto constraints = std::move(source_data.second);
    {
        std::lock_guard<std::mutex> lock(share.primary_vocabulary.mutex);

        auto& dic     = share.primary_vocabulary.data;
        auto& lattice = *dic->lattice;
        lattice.set_request_type(best_only ? MECAB_ONE_BEST : MECAB_NBEST);
        lattice.set_sentence(buffer.data());
        set_constraints(lattice, constraints);
        dic->tagger->parse(&lattice);
        std::vector<std::string> result_features;
        while(1) {
            auto        parsed_nodes   = parse_nodes(lattice, true);
            std::string parsed_feature = std::move(parsed_nodes.first);
            Phrases     parsed         = std::move(parsed_nodes.second);
            bool        matched        = false;
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
        size_t total_bytes          = 0;
        auto   protected_phrase_pos = constraints.cbegin();
        for(auto& p : r) {
            if(protected_phrase_pos == constraints.cend()) {
                break;
            }
            if(total_bytes == protected_phrase_pos->begin) {
                // this is a phrase from a protected one.
                p.set_protection_level(protected_phrase_pos->phrase->get_protection_level());
                if(p.get_protection_level() == ProtectionLevel::PRESERVE_TRANSLATION) {
                    p = *protected_phrase_pos->phrase;
                }
                protected_phrase_pos += 1;
            } else if(total_bytes > protected_phrase_pos->begin) {
                throw std::runtime_error("protected phrase lost.");
            }
            total_bytes += p.get_raw().get_feature().size();
        }
    }

    return result;
}
void MikanEngine::request_fix_dictionary(Phrases wants) {
    std::lock_guard<std::mutex> lock(fix_requests.mutex);
    fix_requests.data.emplace_back(wants);
    dictionary_update_event.wakeup();
}
void MikanEngine::keyEvent(const fcitx::InputMethodEntry& entry, fcitx::KeyEvent& event) {
    auto& context = *event.inputContext();
    auto  state   = context.propertyFor(&factory);
    state->handle_key_event(event);
}
void MikanEngine::activate(const fcitx::InputMethodEntry& entry, fcitx::InputContextEvent& event) {
    auto& context = *event.inputContext();
    auto  state   = context.propertyFor(&factory);
    state->handle_activate();
    event.accept();
}
void MikanEngine::deactivate(const fcitx::InputMethodEntry& entry, fcitx::InputContextEvent& event) {
    auto& context = *event.inputContext();
    auto  state   = context.propertyFor(&factory);
    state->handle_deactivate();
    event.accept();
    reset(entry, event);
}
MikanEngine::MikanEngine(fcitx::Instance* instance) : instance(instance),
                                                      factory([this](fcitx::InputContext& context) {
                                                          auto new_state = new MikanState(context, *this, share);
                                                          return new_state;
                                                      }) {
    instance->inputContextManager().registerProperty("mikan", &factory);

    // load configuration
    if(!load_configuration()) {
        throw std::runtime_error("failed to load configuration.");
    }
    for(const auto& entry : std::filesystem::directory_iterator(DICTIONARY_PATH)) {
        if(entry.path().filename() == "system") {
            system_dictionary_path = entry.path().string();
        } else {
            MeCabModel* new_dic;
            try {
                new_dic = new MeCabModel(entry.path().string().data());
            } catch(const std::runtime_error&) {
                FCITX_WARN() << "failed to load addtional dictionary. " << entry.path();
                continue;
            }
            share.additional_vocabularies.emplace_back(new_dic);
        }
    }
    if(system_dictionary_path.empty()) {
        throw std::runtime_error("failed to find system dictionary.");
    }
    auto cache_dir = get_user_cache_dir();
    if(!std::filesystem::is_directory(cache_dir)) {
        std::filesystem::create_directories(cache_dir);
    }
    history_file_path       = cache_dir + "history.csv";
    history_dictionary_path = cache_dir + "history.dic";
    auto compiler_path      = get_dictionary_compiler_path();
    if(compiler_path.has_value()) {
        dictionary_compiler_path = compiler_path.value() + "/mecab-dict-index";
    }

    histories = load_histories(history_file_path.data());
    compile_user_dictionary();
    reload_dictionary();

    finish_dictionary_updater = false;
    dictionary_updater        = std::thread([this]() {
        while(!finish_dictionary_updater) {
            Phrases request;
            do {
                {
                    std::lock_guard<std::mutex> lock(fix_requests.mutex);
                    if(fix_requests.data.empty()) {
                        break;
                    }
                    request = std::move(fix_requests.data[0]);
                    fix_requests.data.erase(fix_requests.data.begin());
                }
                compare_and_fix_dictionary(request);
            } while(0);
            {
                std::lock_guard<std::mutex> lock(fix_requests.mutex);
                if(!fix_requests.data.empty()) {
                    continue;
                }
            }
            dictionary_update_event.wait();
        }
    });

    share.key_config.keys.resize(static_cast<size_t>(Actions::ACTIONS_LIMIT));
    share.key_config[Actions::BACKSPACE]            = {{FcitxKey_BackSpace}};
    share.key_config[Actions::REINTERPRET_NEXT]     = {{FcitxKey_space}};
    share.key_config[Actions::REINTERPRET_PREV]     = {{FcitxKey_space, fcitx::KeyState::Shift}};
    share.key_config[Actions::CANDIDATE_NEXT]       = {{FcitxKey_Down}, {FcitxKey_J, fcitx::KeyState::Ctrl}};
    share.key_config[Actions::CANDIDATE_PREV]       = {{FcitxKey_Up}, {FcitxKey_K, fcitx::KeyState::Ctrl}};
    share.key_config[Actions::CANDIDATE_PAGE_NEXT]  = {{FcitxKey_Left}};
    share.key_config[Actions::CANDIDATE_PAGE_PREV]  = {{FcitxKey_Right}};
    share.key_config[Actions::COMMIT]               = {{FcitxKey_Return}};
    share.key_config[Actions::PHRASE_NEXT]          = {{FcitxKey_Left}, {FcitxKey_H, fcitx::KeyState::Ctrl}};
    share.key_config[Actions::PHRASE_PREV]          = {{FcitxKey_Right}, {FcitxKey_L, fcitx::KeyState::Ctrl}};
    share.key_config[Actions::SPLIT_PHRASE_LEFT]    = {{FcitxKey_Left, fcitx::KeyState::Ctrl}, {FcitxKey_H, fcitx::KeyState::Alt}};
    share.key_config[Actions::SPLIT_PHRASE_RIGHT]   = {{FcitxKey_Right, fcitx::KeyState::Ctrl}, {FcitxKey_L, fcitx::KeyState::Alt}};
    share.key_config[Actions::MERGE_PHRASE_LEFT]    = {{FcitxKey_Left, fcitx::KeyState::Ctrl_Alt}, {FcitxKey_J, fcitx::KeyState::Alt}};
    share.key_config[Actions::MERGE_PHRASE_RIGHT]   = {{FcitxKey_Right, fcitx::KeyState::Ctrl_Alt}, {FcitxKey_K, fcitx::KeyState::Alt}};
    share.key_config[Actions::MOVE_SEPARATOR_LEFT]  = {{FcitxKey_Left, fcitx::KeyState::Alt}, {FcitxKey_H, fcitx::KeyState::Ctrl_Alt}};
    share.key_config[Actions::MOVE_SEPARATOR_RIGHT] = {{FcitxKey_Right, fcitx::KeyState::Alt}, {FcitxKey_L, fcitx::KeyState::Ctrl_Alt}};
    share.key_config[Actions::CONVERT_KATAKANA]     = {{FcitxKey_q}};
}
MikanEngine::~MikanEngine() {
    finish_dictionary_updater = true;
    dictionary_update_event.wakeup();
    dictionary_updater.join();
    delete share.primary_vocabulary.data;
    for(auto d : share.additional_vocabularies) {
        delete d;
    }
    save_hisotry();
}

class MikanEngineFactory : public fcitx::AddonFactory {
  public:
    fcitx::AddonInstance* create(fcitx::AddonManager* manager) override;
};
fcitx::AddonInstance* MikanEngineFactory::create(fcitx::AddonManager* manager) {
    //    fcitx::registerDomain("mikan", FCITX_INSTALL_LOCALEDIR);
    return new MikanEngine(manager->instance());
}
FCITX_ADDON_FACTORY(MikanEngineFactory);
