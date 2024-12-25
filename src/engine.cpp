#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <unordered_set>

#include "engine.hpp"
#include "macros/unwrap.hpp"
#include "misc.hpp"
#include "tmp.hpp"
#include "util/charconv.hpp"
#include "util/split.hpp"
#include "util/string-map.hpp"

namespace {
auto split_strip(const std::string_view str, const std::string_view dlm = " ") -> std::vector<std::string_view> {
    auto vec = split(str, dlm);
    std::erase(vec, "");
    return vec;
}

using StringSet = std::unordered_set<std::string, internal::StringHash, std::ranges::equal_to>;
} // namespace

namespace mikan::engine {
namespace {
auto parse_line_to_history(const std::string_view line) -> std::optional<History> {
    const auto elms = split_strip(line, ",");
    auto       hist = History();
    if(elms.size() == 2) {
        hist.raw       = elms[0];
        hist.converted = MeCabWord(std::string(elms[1]));
    } else if(elms.size() == 5) {
        hist.raw = elms[0];
        unwrap(rattr, from_chars<unsigned short>(elms[1]));
        unwrap(lattr, from_chars<unsigned short>(elms[2]));
        unwrap(cost, from_chars<short>(elms[3]));
        hist.converted = MeCabWord(std::string(elms[4]), WordParameters{rattr, lattr, cost, 0});
    } else {
        bail();
    }
    return hist;
}

auto parse_line_to_key_value(const std::string_view line) -> std::pair<std::string_view, std::string_view> {
    const auto vec = split_strip(line);
    if(vec.size() != 2) {
        return {};
    }
    return {vec[0], vec[1]};
}

auto build_raw_and_constraints(const WordChain& chain, const bool ignore_protection) -> std::pair<std::string, std::vector<FeatureConstriant>> {
    auto feature_constriants = std::vector<FeatureConstriant>();
    auto buffer              = std::string();
    for(const auto& word : chain) {
        const auto& raw = word.raw();
        if(!ignore_protection && word.protection != ProtectionLevel::None) {
            auto begin = buffer.size();
            auto end   = begin + raw.size();
            feature_constriants.emplace_back(FeatureConstriant{begin, end, &word});
        };
        buffer += raw;
    }
    return std::make_pair(buffer, feature_constriants);
}

auto load_text_dictionary(const char* const path) -> std::vector<History> {
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

auto set_constraints(MeCab::Lattice& lattice, const std::vector<FeatureConstriant>& constraints) -> void {
    for(const auto& c : constraints) {
        const auto& word    = *c.word;
        const auto  feature = word.protection == ProtectionLevel::PreserveTranslation ? word.feature().data() : "*";
        lattice.set_feature_constraint(c.begin, c.end, feature);
    }
}

auto parse_nodes(MeCab::Lattice& lattice, const bool needs_parsed_feature) -> std::pair<std::string, WordChain> {
    auto parsed         = WordChain();
    auto parsed_feature = std::string();
    for(const auto* node = lattice.bos_node(); node; node = node->next) {
        if(node->stat == MECAB_BOS_NODE || node->stat == MECAB_EOS_NODE) {
            continue;
        }
        parsed.emplace_back(Word::from_node(*node));
        if(needs_parsed_feature) {
            parsed_feature += node->feature;
        }
    }
    return std::make_pair(parsed_feature, parsed);
}
} // namespace

auto Engine::load_configuration() -> bool {
    const auto user_config_dir = get_user_config_dir();
    ensure(std::filesystem::is_directory(user_config_dir) || std::filesystem::create_directories(user_config_dir));
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

auto Engine::add_history(const History& word) -> bool {
    auto have_new_word    = true;
    auto cost_not_changed = true;
    auto duplicates       = size_t(0);
    for(auto& hist : histories) {
        auto& lword = hist.converted;
        auto& rword = word.converted;
        if(hist.raw != word.raw || lword.params != lword.params) {
            continue;
        }
        if(lword.feature == rword.feature) {
            if(lword.params->word_cost != rword.params->word_cost) {
                lword.params->word_cost = rword.params->word_cost;
                cost_not_changed        = false;
            }
            have_new_word = false;
            break;
        } else {
            duplicates += 1;
        }
    }

    if(have_new_word) {
        histories.emplace_back(word);
        if(duplicates != 0) {
            auto& w = histories.back().converted;
            w.params->word_cost -= duplicates;
        }
    }
    return have_new_word || !cost_not_changed;
}

auto Engine::save_hisotry() const -> void {
    auto file = std::ofstream(history_file_path);
    for(const auto& hist : histories) {
        auto line = std::string();
        line += hist.raw + ",";
        const auto& word = hist.converted;
        if(word.params) {
            line += std::to_string(word.params->cattr_left) + ",";
            line += std::to_string(word.params->cattr_right) + ",";
            line += std::to_string(word.params->word_cost) + ",";
        }
        line += word.feature;
        file << line << std::endl;
    }
}

auto Engine::dump_internal_dict(const char* const path) const -> void {
    auto to_compile = std::vector<History>();
    for(const auto& dict : user_dictionary_paths) {
        auto hists = load_text_dictionary(dict.data());
        std::copy(hists.begin(), hists.end(), std::back_inserter(to_compile));
    }
    std::copy(histories.begin(), histories.end(), std::back_inserter(to_compile));
    auto file = std::ofstream(path);
    for(const auto& hist : to_compile) {
        const auto& word   = hist.converted;
        const auto  params = word.params ? *word.params : WordParameters();
        file << build_string(hist.raw, ",", params.cattr_left, ",", params.cattr_right, ",", params.word_cost, ",", word.feature) << std::endl;
    }
}

auto Engine::compile_and_reload_user_dictionary() -> void {
    auto       tmp = TemporaryDirectory();
    const auto csv = tmp.str() + "/dict.csv";
    const auto bin = tmp.str() + "/dict.bin";
    dump_internal_dict(csv.data());

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

auto Engine::reload_dictionary(const char* const user_dict) -> void {
    share.primary_vocabulary = std::make_shared<MeCabModel>(system_dictionary_path.data(), user_dict, true);
}

auto Engine::convert_wordchain(const WordChain& source, const bool best_only, const bool ignore_protection) const -> WordChains {
    constexpr auto N_BEST_LIMIT = size_t(30);

    auto result                   = WordChains();
    const auto [raw, constraints] = build_raw_and_constraints(source, ignore_protection);
    {
        auto  dic     = share.primary_vocabulary;
        auto& lattice = *dic->lattice;
        lattice.set_request_type(best_only ? MECAB_ONE_BEST : MECAB_NBEST);
        lattice.set_sentence(raw.data());
        set_constraints(lattice, constraints);
        dic->tagger->parse(&lattice);

        auto found_features = StringSet();
        while(1) {
            auto [features, parsed] = parse_nodes(lattice, true);
            if(!found_features.contains(features)) {
                result.emplace_back(parsed);
                found_features.emplace(std::move(features));
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

    // retrive protected wordchain
    for(auto& r : result) {
        auto total_bytes = size_t(0);
        auto constraint  = constraints.cbegin();
        for(auto& word : r) {
            if(constraint == constraints.cend()) {
                break;
            }
            if(total_bytes == constraint->begin) {
                // this is a word from a protected one
                word.protection = constraint->word->protection;
                if(word.protection == ProtectionLevel::PreserveTranslation) {
                    word = *constraint->word;
                }
                constraint += 1;
            } else if(total_bytes > constraint->begin) {
                PANIC("protected word lost");
            }
            total_bytes += word.raw().size();
        }
    }

    return result;
}

Engine::Engine(Share& share)
    : share(share) {
    ASSERT(load_configuration(), "failed to load configuration");
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
    ASSERT(!system_dictionary_path.empty(), "failed to find system dictionary");

    if(const auto compiler_path = get_dictionary_compiler_path()) {
        dictionary_compiler_path = compiler_path.value() + "/mecab-dict-index";
        compile_and_reload_user_dictionary();
    } else {
        FCITX_WARN() << "missing dictionary compiler";
        reload_dictionary();
    }

    share.key_config.keys.resize(static_cast<size_t>(Actions::ActionsLimit));
    share.key_config[Actions::Backspace]         = {{FcitxKey_BackSpace}};
    share.key_config[Actions::ReinterpretNext]   = {{FcitxKey_space}};
    share.key_config[Actions::ReinterpretPrev]   = {{FcitxKey_space, fcitx::KeyState::Shift}};
    share.key_config[Actions::CandidateNext]     = {{FcitxKey_Down}, {FcitxKey_J, fcitx::KeyState::Ctrl}};
    share.key_config[Actions::CandidatePrev]     = {{FcitxKey_Up}, {FcitxKey_K, fcitx::KeyState::Ctrl}};
    share.key_config[Actions::CandidatePageNext] = {{FcitxKey_Left}};
    share.key_config[Actions::CandidatePagePrev] = {{FcitxKey_Right}};
    share.key_config[Actions::Commit]            = {{FcitxKey_Return}};
    share.key_config[Actions::WordNext]          = {{FcitxKey_Left}, {FcitxKey_L, fcitx::KeyState::Ctrl}};
    share.key_config[Actions::WordPrev]          = {{FcitxKey_Right}, {FcitxKey_H, fcitx::KeyState::Ctrl}};
    share.key_config[Actions::SplitWordLeft]     = {{FcitxKey_H, fcitx::KeyState::Alt}};
    share.key_config[Actions::SplitWordRight]    = {{FcitxKey_L, fcitx::KeyState::Alt}};
    share.key_config[Actions::MergeWordsLeft]    = {{FcitxKey_J, fcitx::KeyState::Alt}};
    share.key_config[Actions::MergeWordsRight]   = {{FcitxKey_K, fcitx::KeyState::Alt}};
    share.key_config[Actions::GiveToLeft]        = {{FcitxKey_J, fcitx::KeyState::Ctrl_Alt}};
    share.key_config[Actions::GiveToRight]       = {{FcitxKey_K, fcitx::KeyState::Ctrl_Alt}};
    share.key_config[Actions::TakeFromLeft]      = {{FcitxKey_H, fcitx::KeyState::Ctrl_Alt}};
    share.key_config[Actions::TakeFromRight]     = {{FcitxKey_L, fcitx::KeyState::Ctrl_Alt}};
    share.key_config[Actions::ConvertKatakana]   = {{FcitxKey_q}}; // not Q
}
} // namespace mikan::engine
