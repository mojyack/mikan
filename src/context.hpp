#pragma once
#include <fcitx-utils/utf8.h>
#include <fcitx/inputcontext.h>
#include <fcitx/inputcontextproperty.h>
#include <fcitx/inputpanel.h>

#include "command.hpp"
#include "engine.hpp"
#include "romaji-index.hpp"
#include "share.hpp"
#include "wordchain-candidates.hpp"

namespace mikan {
class Context final : public fcitx::InputContextProperty {
  public:
    fcitx::InputContext& context;
    engine::Engine&      engine;
    Share&               share;

  private:
    size_t              cursor; // word index
    RomajiIndex         romaji_index;
    std::string         to_kana;
    WordChainCandidates chains;

    std::optional<CommandModeContext> command_mode_context;

    auto get_current_chain() -> WordChain&;
    auto get_current_chain() const -> const WordChain&;
    auto commit_word(const Word& word) -> void;
    auto commit_wordchain() -> void;
    auto translate_wordchain(const WordChain& source, bool best_only) -> WordChains;
    auto build_preedit_text() const -> fcitx::Text;
    auto build_kana_text() const -> std::string;
    auto apply_candidates() -> void;
    auto auto_commit() -> void;
    auto handle_key_event_normal(fcitx::KeyEvent& event) -> void;
    auto handle_deactivate_normal() -> void;

    auto exit_command_mode() -> void;
    auto handle_key_event_command(fcitx::KeyEvent& event) -> void;
    auto handle_deactivate_command() -> void;

  public:
    auto handle_key_event(fcitx::KeyEvent& event) -> void;
    auto handle_activate() -> void;
    auto handle_deactivate() -> void;

    Context(fcitx::InputContext& context, engine::Engine& engine, Share& share);
};
} // namespace mikan
