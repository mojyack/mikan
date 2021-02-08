#pragma once
#include <thread>

#include <fcitx/event.h>
#include <fcitx/inputmethodengine.h>
#include <fcitx/instance.h>

#include "state.hpp"
#include "type.hpp"

class MikanContext;
class History;
class MikanEngine final : public fcitx::InputMethodEngine {
  private:
    using DictionaryWord  = std::array<std::string, 2>;
    using DictionaryWords = std::vector<DictionaryWord>;

    fcitx::Instance*              instance;
    fcitx::FactoryFor<MikanState> factory;
    Share                         share;

    std::string                   system_dictionary_path;
    std::string                   history_file_path;
    std::string                   history_dictionary_path;
    std::string                   dictionary_compiler_path;
    std::vector<std::string>      user_dictionary_paths;
    std::thread                   dictionary_updater;
    bool                          finish_dictionary_updater;
    ConditionalVariable           dictionary_update_event;
    SafeVar<DictionaryWords>      dictionary_update_queue;
    SafeVar<std::vector<History>> histories;

    bool load_configuration();
    bool add_history(const DictionaryWords& words);
    void save_hisotry() const;
    bool compile_user_dictionary() const;
    void reload_dictionary();

  public:
    void request_update_dictionary(const char* raw, const char* translation);

    void keyEvent(const fcitx::InputMethodEntry& entry, fcitx::KeyEvent& event) override;
    void activate(const fcitx::InputMethodEntry& entry, fcitx::InputContextEvent& event) override;
    void deactivate(const fcitx::InputMethodEntry& entry, fcitx::InputContextEvent& event) override;
    MikanEngine(fcitx::Instance* instance);
    ~MikanEngine();
};
