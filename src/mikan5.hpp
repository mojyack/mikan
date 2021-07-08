#pragma once
#include <fcitx/inputmethodengine.h>
#include <fcitx/instance.h>

#include "state.hpp"

namespace mikan {
class MikanContext;

struct History {
    std::string raw;
    MeCabWord   translation;
};

using Histories = std::vector<History>;

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

    auto load_configuration() -> bool;
    auto add_history(const History& word) -> bool;
    auto save_hisotry() const -> void;
    auto compile_user_dictionary() const -> bool;
    auto reload_dictionary() -> void;
    auto recalc_cost(const Phrases& phrases) const -> long;
    auto compare_and_fix_dictionary(const Phrases& wants) -> void;

  public:
    auto translate_phrases(const Phrases& source, bool best_only, bool ignore_protection = false) const -> Sentences;
    auto request_fix_dictionary(Phrases wants) -> void;

    auto keyEvent(const fcitx::InputMethodEntry& entry, fcitx::KeyEvent& event) -> void override;
    auto activate(const fcitx::InputMethodEntry& entry, fcitx::InputContextEvent& event) -> void override;
    auto deactivate(const fcitx::InputMethodEntry& entry, fcitx::InputContextEvent& event) -> void override;
    MikanEngine(fcitx::Instance* instance);
    ~MikanEngine();
};
} // namespace mikan
