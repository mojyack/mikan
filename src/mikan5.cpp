#include <filesystem>
#include <fstream>

#include <fcitx/addonfactory.h>
#include <fcitx/addonmanager.h>

#include "config.h"
#include "mikan5.hpp"
#include "misc.hpp"

namespace mikan {
namespace {
auto parse_line_to_history(const std::string& line) -> std::optional<History> {
    auto comma_index = size_t(0);
    auto commas      = std::array<size_t, 4>();
    auto pos         = line.find(',');
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
    auto hist = History{.raw = std::string(line.begin() + 0, line.begin() + commas[0])};
    if(comma_index == 1) {
        hist.translation = MeCabWord(std::string(line.begin() + commas[0] + 1, line.end()));
    } else {
        using Attr = unsigned short;
        auto rattr = Attr(), lattr = Attr();
        auto cost = short();
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
struct FeatureConstriant {
    size_t        begin;
    size_t        end;
    const Phrase* phrase;
};
auto build_raw_and_constraints(const Phrases& source, const bool ignore_protection) -> std::pair<std::string, std::vector<FeatureConstriant>> {
    auto feature_constriants = std::vector<FeatureConstriant>();
    auto buffer              = std::string();
    for(auto p = source.begin(); p != source.end(); p += 1) {
        const auto& raw = p->get_raw().get_feature();
        if(!ignore_protection && p->get_protection_level() != ProtectionLevel::NONE) {
            auto begin = buffer.size();
            auto end   = begin + raw.size();
            feature_constriants.emplace_back(FeatureConstriant{begin, end, &*p});
        };
        buffer += raw;
    }
    return std::make_pair(buffer, feature_constriants);
}
auto load_histories(const char* path) -> std::vector<History> {
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
} // namespace
auto MikanEngine::load_configuration() -> bool {
    constexpr auto CONFIG_FILE_NAME              = "mikan.conf";
    constexpr auto CANDIDATE_PAGE_SIZE           = "candidate_page_size";
    constexpr auto AUTO_COMMIT_THRESHOLD         = "auto_commit_threshold";
    constexpr auto DICTIONARY                    = "dictionary";
    constexpr auto INSERT_SPACE                  = "insert_space";
    constexpr auto DEFAULT_CANDIDATE_PAGE_SIZE   = 10;
    constexpr auto DEFAULT_AUTO_COMMIT_THRESHOLD = 8;
    const auto     user_config_dir               = get_user_config_dir();
    if(!std::filesystem::is_directory(user_config_dir) && !std::filesystem::create_directories(user_config_dir)) {
        return false;
    }
    share.candidate_page_size   = DEFAULT_CANDIDATE_PAGE_SIZE;
    share.auto_commit_threshold = DEFAULT_AUTO_COMMIT_THRESHOLD;
    {
        auto config = std::fstream(user_config_dir + CONFIG_FILE_NAME);
        auto l      = std::string();
        while(std::getline(config, l)) {
            auto entry_name = std::string(), value = std::string();
            {
                const auto space = l.find(' ');
                if(space == std::string::npos) {
                    continue;
                }
                entry_name = l.substr(0, space);
                for(auto i = space; l[i] != '\0'; i += 1) {
                    if(space != ' ' && space != '\t') {
                        break;
                    }
                }
                value = l.substr(space + 1);
            }
            auto error = false;
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
                const auto option = value == "on" ? InsertSpaceOptions::On : value == "off" ? InsertSpaceOptions::Off
                                                                         : value == "smart" ? InsertSpaceOptions::Smart
                                                                                            : InsertSpaceOptions::None;
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
auto MikanEngine::add_history(const History& word) -> bool {
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
void MikanEngine::save_hisotry() const {
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
bool MikanEngine::compile_user_dictionary() const {
    auto csv = std::string("/tmp/mikan-tmp");
    while(std::filesystem::exists(csv)) {
        csv += '.';
    }

    {
        auto to_compile = std::vector<History>();
        for(const auto& d : user_dictionary_paths) {
            auto hists = load_histories(d.data());
            std::copy(hists.begin(), hists.end(), std::back_inserter(to_compile));
        }
        std::copy(histories.begin(), histories.end(), std::back_inserter(to_compile));
        auto file = std::ofstream(csv);
        for(const auto& l : to_compile) {
            const auto& t     = l.translation;
            const auto  lattr = t.has_valid_word_info() ? t.get_cattr_left() : 0;
            const auto  rattr = t.has_valid_word_info() ? t.get_cattr_right() : 0;
            const auto  cost  = t.has_valid_word_info() ? t.get_word_cost() : 0;
            file << l.raw << "," << std::to_string(lattr) << "," << std::to_string(rattr) << "," << std::to_string(cost) << "," << t.get_feature() << std::endl;
        }
    }

    auto command = std::string();
    command += dictionary_compiler_path;
    command += " -d ";
    command += system_dictionary_path;
    command += " -u ";
    command += history_dictionary_path;
    command += " -f utf-8 -t utf-8 ";
    command += csv;
    command += " >& /dev/null";
    const auto exit_code = WEXITSTATUS(system(command.data()));
    std::filesystem::remove(csv);
    return exit_code == 0;
}
auto set_constraints(MeCab::Lattice& lattice, const std::vector<FeatureConstriant>& constraints) -> void {
    for(const auto& f : constraints) {
        auto& phrase = *f.phrase;
        lattice.set_feature_constraint(f.begin, f.end, phrase.get_protection_level() == ProtectionLevel::PRESERVE_TRANSLATION ? phrase.get_translated().get_feature().data() : "*");
    }
}
auto parse_nodes(MeCab::Lattice& lattice, const bool needs_parsed_feature) -> std::pair<std::string, Phrases> {
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
void MikanEngine::reload_dictionary() {
    auto new_dic = new MeCabModel(system_dictionary_path.data(), history_dictionary_path.data(), true);
    auto old_dic = share.primary_vocabulary.load();
    share.primary_vocabulary.store(new_dic);
    if(old_dic != nullptr) {
        delete old_dic;
    }
}
auto MikanEngine::recalc_cost(const Phrases& source) const -> long {
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

    const auto lock    = share.primary_vocabulary.get_lock();
    auto&      dic     = share.primary_vocabulary.data;
    auto&      lattice = *dic->lattice;
    lattice.set_request_type(MECAB_ONE_BEST);
    lattice.set_sentence(buffer.data());
    set_constraints(lattice, constraints);
    dic->tagger->parse(&lattice);
    return parse_nodes(lattice, true).second.back().get_translated().get_total_cost();
}
auto MikanEngine::compare_and_fix_dictionary(const Phrases& wants) -> void {
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
            compile_user_dictionary();
            reload_dictionary();
        }
    }

    const auto source_cost = recalc_cost(wants);
    while(1) {
        auto to_decrements      = Histories();
        auto best               = translate_phrases(wants, true, true)[0];
        auto all_phrases_passed = true;
        for(auto& p : best) {
            p.set_protection_level(ProtectionLevel::NONE);
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
            pb.set_protection_level(ProtectionLevel::PRESERVE_TRANSLATION);
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
        compile_user_dictionary();
        reload_dictionary();
    }
}
Sentences MikanEngine::translate_phrases(const Phrases& source, const bool best_only, const bool ignore_protection) const {
    constexpr auto N_BEST_LIMIT = size_t(30);

    auto       result      = Sentences();
    auto       source_data = build_raw_and_constraints(source, ignore_protection);
    const auto buffer      = std::move(source_data.first);
    const auto constraints = std::move(source_data.second);
    {
        const auto lock = share.primary_vocabulary.get_lock();

        const auto& dic     = share.primary_vocabulary.data;
        auto&       lattice = *dic->lattice;
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
auto MikanEngine::request_fix_dictionary(Phrases wants) -> void {
    const auto lock = fix_requests.get_lock();
    fix_requests.data.emplace_back(wants);
    dictionary_update_event.wakeup();
}
auto MikanEngine::keyEvent(const fcitx::InputMethodEntry& entry, fcitx::KeyEvent& event) -> void {
    auto& context = *event.inputContext();
    auto  state   = context.propertyFor(&factory);
    state->handle_key_event(event);
}
auto MikanEngine::activate(const fcitx::InputMethodEntry& entry, fcitx::InputContextEvent& event) -> void {
    auto& context = *event.inputContext();
    auto  state   = context.propertyFor(&factory);
    state->handle_activate();
    event.accept();
}
auto MikanEngine::deactivate(const fcitx::InputMethodEntry& entry, fcitx::InputContextEvent& event) -> void {
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
            auto new_dic = (MeCabModel*)(nullptr);
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
    const auto cache_dir = get_user_cache_dir();
    if(!std::filesystem::is_directory(cache_dir)) {
        std::filesystem::create_directories(cache_dir);
    }
    history_file_path       = cache_dir + "history.csv";
    history_dictionary_path = cache_dir + "history.dic";

    const auto compiler_path = get_dictionary_compiler_path();
    if(compiler_path.has_value()) {
        dictionary_compiler_path = compiler_path.value() + "/mecab-dict-index";
    }

    histories = load_histories(history_file_path.data());
    compile_user_dictionary();
    reload_dictionary();

    finish_dictionary_updater = false;
    dictionary_updater        = std::thread([this]() {
        while(!finish_dictionary_updater) {
            auto request = Phrases();
            do {
                {
                    const auto lock = fix_requests.get_lock();
                    if(fix_requests.data.empty()) {
                        break;
                    }
                    request = std::move(fix_requests.data[0]);
                    fix_requests.data.erase(fix_requests.data.begin());
                }
                compare_and_fix_dictionary(request);
            } while(0);
            {
                const auto lock = fix_requests.get_lock();
                if(!fix_requests.data.empty()) {
                    continue;
                }
            }
            dictionary_update_event.wait();
        }
           });

    share.key_config.keys.resize(static_cast<size_t>(Actions::Actions_limit));
    share.key_config[Actions::Backspace]            = {{FcitxKey_BackSpace}};
    share.key_config[Actions::Reinterpret_next]     = {{FcitxKey_space}};
    share.key_config[Actions::Reinterpret_prev]     = {{FcitxKey_space, fcitx::KeyState::Shift}};
    share.key_config[Actions::Candidate_next]       = {{FcitxKey_Down}, {FcitxKey_J, fcitx::KeyState::Ctrl}};
    share.key_config[Actions::Candidate_prev]       = {{FcitxKey_Up}, {FcitxKey_K, fcitx::KeyState::Ctrl}};
    share.key_config[Actions::Candidate_page_next]  = {{FcitxKey_Left}};
    share.key_config[Actions::Candidate_page_prev]  = {{FcitxKey_Right}};
    share.key_config[Actions::Commit]               = {{FcitxKey_Return}};
    share.key_config[Actions::Phrase_next]          = {{FcitxKey_Left}, {FcitxKey_H, fcitx::KeyState::Ctrl}};
    share.key_config[Actions::Phrase_prev]          = {{FcitxKey_Right}, {FcitxKey_L, fcitx::KeyState::Ctrl}};
    share.key_config[Actions::Split_phrase_left]    = {{FcitxKey_Left, fcitx::KeyState::Ctrl}, {FcitxKey_H, fcitx::KeyState::Alt}};
    share.key_config[Actions::Split_phrase_right]   = {{FcitxKey_Right, fcitx::KeyState::Ctrl}, {FcitxKey_L, fcitx::KeyState::Alt}};
    share.key_config[Actions::Merge_phrase_left]    = {{FcitxKey_Left, fcitx::KeyState::Ctrl_Alt}, {FcitxKey_J, fcitx::KeyState::Alt}};
    share.key_config[Actions::Merge_phrase_right]   = {{FcitxKey_Right, fcitx::KeyState::Ctrl_Alt}, {FcitxKey_K, fcitx::KeyState::Alt}};
    share.key_config[Actions::Move_separator_left]  = {{FcitxKey_Left, fcitx::KeyState::Alt}, {FcitxKey_H, fcitx::KeyState::Ctrl_Alt}};
    share.key_config[Actions::Move_separator_right] = {{FcitxKey_Right, fcitx::KeyState::Alt}, {FcitxKey_L, fcitx::KeyState::Ctrl_Alt}};
    share.key_config[Actions::Convert_katakana]     = {{FcitxKey_q}};
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
    auto create(fcitx::AddonManager* manager) -> fcitx::AddonInstance* override {
        // fcitx::registerDomain("mikan", FCITX_INSTALL_LOCALEDIR);
        return new MikanEngine(manager->instance());
    }
};
} // namespace mikan

FCITX_ADDON_FACTORY(mikan::MikanEngineFactory);
