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
#include <vector>

#include <bits/stdint-uintn.h>
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
#include "romaji-table.hpp"
#include "state.hpp"

class History {
  public:
    std::string raw;
    std::string translation;
    int         cost;
};

std::optional<History> parse_line_to_history_cost(const std::string& line) {
    size_t comma_index = 0;
    size_t commas[2];
    auto   pos = line.find(',');
    while(pos != std::string::npos) {
        if(comma_index >= 2) {
            comma_index = 0;
            break;
        }
        commas[comma_index] = pos;
        comma_index += 1;
        pos = line.find(',', pos + 1);
    }
    if(comma_index != 2) {
        // error.
        return std::nullopt;
    }
    History hist;
    try {
        hist.cost = std::stoi(std::string(line.begin() + commas[1] + 1, line.end()));
    } catch(const std::invalid_argument&) {
        // error.
        return std::nullopt;
    }
    hist.raw         = std::string(line.begin() + 0, line.begin() + commas[0]);
    hist.translation = std::string(line.begin() + commas[0] + 1, line.begin() + commas[1]);
    if(hist.raw.empty() || hist.translation.empty()) {
        return std::nullopt;
    }
    return hist;
}
std::optional<History> parse_line_to_history(const std::string& line) {
    size_t comma = line.find(',');
    if(comma == std::string::npos) {
        return std::nullopt;
    }

    History hist;
    hist.cost        = 0;
    hist.raw         = std::string(line.begin() + 0, line.begin() + comma);
    hist.translation = std::string(line.begin() + comma + 1, line.end());
    if(hist.raw.empty() || hist.translation.empty()) {
        return std::nullopt;
    }
    return hist;
}
std::vector<History> load_histories(const char* path, bool has_cost) {
    std::vector<History> res;
    if(!std::filesystem::is_regular_file(path)) {
        return res;
    }
    std::fstream source(path);
    std::string  l;
    while(std::getline(source, l)) {
        auto hist = has_cost ? parse_line_to_history_cost(l) : parse_line_to_history(l);
        if(!hist.has_value()) {
            continue;
        }
        res.emplace_back(hist.value());
    }
    return res;
}
bool MikanEngine::load_configuration() {
    constexpr const char* CONFIG_FILE_NAME              = "mikan.conf";
    constexpr const char* CANDIDATE_PAGE_SIZE           = "candidate_page_size";
    constexpr const char* AUTO_COMMIT_THRESHOLD         = "auto_commit_threshold";
    constexpr const char* DICTIONARY                    = "dictionary";
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
            }
            if(error) {
                FCITX_WARN() << "error while parsing configuration: " << entry_name;
            }
        }
    }
    return true;
}
bool MikanEngine::add_history(const DictionaryWords& words) {
    constexpr int INITIAL_COST     = 0;
    constexpr int COST_INCREMENTAL = 50;

    bool history_not_changed = true;
    for(const auto& w : words) {
        bool                  new_word         = true;
        bool                  word_not_changed = true;
        std::vector<History*> duplicates;
        for(auto& l : histories.data) {
            if(l.raw != w[0]) {
                continue;
            }
            if(l.translation == w[1]) {
                if(l.cost != INITIAL_COST) {
                    l.cost           = INITIAL_COST;
                    word_not_changed = false;
                }
                new_word = false;
                break;
            } else {
                duplicates.emplace_back(&l);
            }
        }

        if(!duplicates.empty()) {
            word_not_changed = false;
            for(auto d : duplicates) {
                d->cost += COST_INCREMENTAL;
            }
        }
        if(new_word) {
            histories.data.emplace_back(History{w[0], w[1], INITIAL_COST});
        }
        history_not_changed &= word_not_changed & !new_word;
    }

    return !history_not_changed;
}
void MikanEngine::save_hisotry() const {
    std::ofstream file(history_file_path);
    for(const auto& l : histories.data) {
        std::string line;
        line += l.raw + ",";
        line += l.translation + ",";
        line += std::to_string(l.cost);
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
            auto hists = load_histories(d.data(), false);
            std::copy(hists.begin(), hists.end(), std::back_inserter(to_compile));
        }
        std::copy(histories.data.begin(), histories.data.end(), std::back_inserter(to_compile));
        std::ofstream file(csv);
        for(const auto& l : to_compile) {
            file << l.raw << ",0,0," << std::to_string(l.cost) << "," << l.translation << std::endl;
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
    auto new_dic = new MeCabModel(system_dictionary_path.data(), history_dictionary_path.data());
    auto old_dic = share.primary_vocabulary.load();
    share.primary_vocabulary.store(new_dic);
    if(old_dic != nullptr) {
        delete old_dic;
    }
}
void MikanEngine::request_update_dictionary(const char* raw, const char* translation) {
    {
        std::lock_guard<std::mutex> lock(dictionary_update_queue.mutex);
        dictionary_update_queue.data.emplace_back(DictionaryWord{raw, translation});
    }
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

    histories.data = load_histories(history_file_path.data(), true);
    compile_user_dictionary();
    reload_dictionary();

    finish_dictionary_updater = false;
    dictionary_updater        = std::thread([this]() {
        while(!finish_dictionary_updater) {
            DictionaryWords queue;
            {
                std::lock_guard<std::mutex> lock(dictionary_update_queue.mutex);
                queue = dictionary_update_queue.data;
                dictionary_update_queue.data.clear();
            }
            do {
                if(queue.empty()) {
                    break;
                }
                std::lock_guard<std::mutex> lock(histories.mutex);
                if(!add_history(queue)) {
                    break;
                }
                compile_user_dictionary();
                reload_dictionary();
            } while(0);
            {
                std::lock_guard<std::mutex> lock(dictionary_update_queue.mutex);
                if(!dictionary_update_queue.data.empty()) {
                    continue;
                }
            }
            dictionary_update_event.wait();
        }
    });

    share.key_config.keys.resize(static_cast<size_t>(Actions::ACTIONS_LIMIT));
    share.key_config[Actions::BACKSPACE]            = {{FcitxKey_BackSpace}};
    share.key_config[Actions::CANDIDATE_NEXT]       = {{FcitxKey_space}, {FcitxKey_Down}, {FcitxKey_J, fcitx::KeyState::Ctrl}};
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
