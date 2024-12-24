#pragma once
#include <fcitx-utils/utf8.h>
#include <fcitx/inputcontext.h>
#include <fcitx/inputcontextproperty.h>
#include <fcitx/inputpanel.h>

#include "engine.hpp"
#include "romaji-index.hpp"
#include "share.hpp"
#include "wordchain-candidates.hpp"

namespace mikan {
class Context final : public fcitx::InputContextProperty {
  private:
    fcitx::InputContext& context;
    engine::Engine&      engine;
    Share&               share;
    size_t               cursor;       // word index
    RomajiIndex          romaji_index; // used in handle_romaji().
    std::string          to_kana;
    WordChainCandidates  chains;
    bool                 translation_changed = false;
    bool                 chain_changed       = false;

    auto is_empty() const -> bool;
    auto get_current_chain() -> WordChain&;
    auto get_current_chain() const -> const WordChain&;
    auto commit_word(const Word& word) -> void;
    auto commit_wordchain() -> void;
    auto make_branch_chain() -> void;
    auto merge_branch_chains() -> bool;
    auto shrink_chains(bool reserve_one = false) -> void;
    auto delete_surrounding_text() -> bool;
    auto translate_wordchain(const WordChain& source, bool best_only) -> WordChains;
    auto build_preedit_text() const -> fcitx::Text;
    auto build_kana_text() const -> std::string;
    auto apply_candidates() -> void;
    auto auto_commit() -> void;
    auto reset() -> void;

  public:
    auto handle_key_event(fcitx::KeyEvent& event) -> void;
    auto handle_activate() -> void;
    auto handle_deactivate() -> void;

    Context(fcitx::InputContext& context, engine::Engine& engine, Share& share);
};
} // namespace mikan
