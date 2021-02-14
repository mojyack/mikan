#pragma once
#include <thread>

#include <fcitx/event.h>
#include <fcitx/inputmethodengine.h>
#include <fcitx/instance.h>

#include "phrase.hpp"
#include "share.hpp"
#include "state.hpp"
#include "type.hpp"

class MikanContext;
class History;
class MikanEngine final : public fcitx::InputMethodEngine {
  private:
    fcitx::Instance*              instance;
    fcitx::FactoryFor<MikanState> factory;
    Share                         share;

    std::string              system_dictionary_path;
    std::string              history_file_path;
    std::string              history_dictionary_path;
    std::string              dictionary_compiler_path;
    std::vector<std::string> user_dictionary_paths;
    std::thread              dictionary_updater;
    bool                     finish_dictionary_updater;
    ConditionalVariable      dictionary_update_event;
    Histories                histories;
    SafeVar<Sentences>       fix_requests;

    bool load_configuration();
    bool add_history(const History& word);
    void save_hisotry() const;
    bool compile_user_dictionary() const;
    void reload_dictionary();
    long recalc_cost(const Phrases& phrases) const;
    void compare_and_fix_dictionary(const Phrases& wants);

  public:
    Sentences translate_phrases(const Phrases& source, bool best_only, bool ignore_protection = false) const;
    void      request_fix_dictionary(Phrases wants);

    void keyEvent(const fcitx::InputMethodEntry& entry, fcitx::KeyEvent& event) override;
    void activate(const fcitx::InputMethodEntry& entry, fcitx::InputContextEvent& event) override;
    void deactivate(const fcitx::InputMethodEntry& entry, fcitx::InputContextEvent& event) override;
    MikanEngine(fcitx::Instance* instance);
    ~MikanEngine();
};
