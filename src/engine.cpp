#include <array>
#include <filesystem>
#include <fstream>
#include <unordered_set>

#include "engine.hpp"
#include "macros/unwrap.hpp"
#include "misc.hpp"
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
auto build_raw_and_constraints(const WordChain& chain, const bool ignore_protection) -> std::pair<std::string, std::vector<FeatureConstriant>> {
    auto feature_constriants = std::vector<FeatureConstriant>();
    auto buffer              = std::string();
    for(const auto& word : chain) {
        const auto& raw = word.raw();
        if(!ignore_protection && word.protection != ProtectionLevel::None) {
            auto begin = buffer.size();
            auto end   = begin + raw.size();
            feature_constriants.emplace_back(FeatureConstriant{begin, end, &word});
        }
        buffer += raw;
    }
    return std::make_pair(buffer, feature_constriants);
}

auto load_text_dictionary(const char* const path, std::vector<ConvDef>& defs) -> bool {
    ensure(std::filesystem::is_regular_file(path), "not a file ", path);
    auto source = std::fstream(path);
    auto line   = std::string();
    while(std::getline(source, line)) {
        const auto elms = split_strip(line, ",");
        auto       def  = ConvDef();
        if(elms.size() != 2) {
            WARN("failed to parse line \"", line, "\" of file ", path);
            continue;
        }
        defs.emplace_back(ConvDef{std::string(elms[0]), std::string(elms[1])});
    }
    return true;
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

auto Engine::parse_configuration_line(const std::string_view line) -> bool {
    if(line.empty() || line[0] == '#') {
        return true;
    }
    const auto elms = split_strip(line);
    ensure(elms.size() == 2);

    const auto key   = elms[0];
    const auto value = elms[1];

    if(key == "candidate_page_size") {
        unwrap(num, from_chars<int>(value));
        share.candidate_page_size = num;
    } else if(key == "auto_commit_threshold") {
        unwrap(num, from_chars<int>(value));
        share.auto_commit_threshold = num;
    } else if(key == "dictionary") {
        emplace_unique(user_dictionary_paths, get_user_config_dir() + "/" + std::string(value));
    } else if(key == "insert_space") {
        if(value == "on") {
            share.insert_space = InsertSpaceOptions::On;
        } else if(value == "off") {
            share.insert_space = InsertSpaceOptions::Off;
        } else if(value == "smart") {
            share.insert_space = InsertSpaceOptions::Smart;
        } else {
            bail("invalid insert_space value ", value);
        }
    } else if(key == "dictionaries") {
        share.dictionary_path = value;
    } else {
        bail("unknown config name ", key);
    }
    return true;
}

auto Engine::load_configuration() -> bool {
    const auto user_config_dir = get_user_config_dir();
    ensure(std::filesystem::is_directory(user_config_dir) || std::filesystem::create_directories(user_config_dir));
    auto config = std::fstream(user_config_dir + "/mikan.conf");
    auto line   = std::string();
    while(std::getline(config, line)) {
        if(!parse_configuration_line(line)) {
            WARN("failed to process configuration ", line);
        }
    }
    return true;
}

auto Engine::merge_dictionaries(const char* const path) const -> bool {
    auto to_compile = std::vector<ConvDef>();

    const auto defines_path = get_user_cache_dir() + "/defines.txt";
    if(std::filesystem::is_regular_file(defines_path)) {
        load_text_dictionary(defines_path.data(), to_compile);
    }

    for(const auto& dict : user_dictionary_paths) {
        load_text_dictionary(dict.data(), to_compile);
    }

    auto file = std::ofstream(path);
    for(const auto& def : to_compile) {
        file << def.raw << ",0,0,0," << def.converted << std::endl;
    }
    return true;
}

auto Engine::compile_and_reload_user_dictionary() -> bool {
    const auto tmpdir = build_string("/tmp/mikan-", getpid());
    std::filesystem::create_directories(tmpdir);

    const auto csv_path = tmpdir + "/dict.csv";
    const auto bin_path = tmpdir + "/dict.bin";
    merge_dictionaries(csv_path.data());

    auto command = std::string();
    command += dictionary_compiler_path;
    command += " -d ";
    command += system_dictionary_path;
    command += " -u ";
    command += bin_path;
    command += " -f utf-8 -t utf-8 ";
    command += csv_path;
    command += " >& /dev/null";
    const auto code = system(command.data());

    reload_dictionary(bin_path.data());

    std::filesystem::remove_all(tmpdir);

    ensure(code == 0);
    return true;
}

auto Engine::reload_dictionary(const char* const user_dict) -> bool {
    share.primary_vocabulary = std::make_shared<MeCabModel>(system_dictionary_path.data(), user_dict, true);
    return true;
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

auto Engine::add_convert_definition(const std::string_view raw, const std::string_view converted) -> bool {
    const auto cachedir = get_user_cache_dir();
    ensure(std::filesystem::is_directory(cachedir) || std::filesystem::create_directories(get_user_cache_dir()));
    const auto path = cachedir + "/defines.txt";

    auto file = std::ofstream(path, std::ios_base::app);
    file << raw << "," << converted << std::endl;

    ensure(compile_and_reload_user_dictionary());
    return true;
}

auto Engine::remove_convert_definition(const std::string_view raw) -> bool {
    const auto path = get_user_cache_dir() + "/defines.txt";

    auto defs = std::vector<ConvDef>();
    ensure(load_text_dictionary(path.data(), defs));

    auto file  = std::ofstream(path, std::ios_base::out);
    auto count = 0;
    for(auto i = defs.begin(); i < defs.end(); i += 1) {
        if(i->raw == raw) {
            count += 1;
            continue;
        }
        file << i->raw << "," << i->converted << std::endl;
    }
    ensure(count > 0, "no definitions matched");
    ensure(compile_and_reload_user_dictionary());
    return true;
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
    share.key_config[Actions::EnterCommandMode]  = {{FcitxKey_slash}};
    share.key_config[Actions::ExitCommandMode]   = {{FcitxKey_Escape}};
}
} // namespace mikan::engine
