#pragma once
#include <fcitx-utils/utf8.h>
#include <fcitx/inputcontext.h>
#include <fcitx/inputcontextproperty.h>
#include <fcitx/inputpanel.h>

#include "engine.hpp"
#include "romaji-index.hpp"
#include "share.hpp"

namespace mikan {
class Context final : public fcitx::InputContextProperty {
  private:
    fcitx::InputContext& context;
    engine::Engine&      engine;
    Share&               share;
    Phrases*             phrases = nullptr;
    size_t               cursor;       // in bytes
    RomajiIndex          romaji_index; // used in handle_romaji().
    std::string          to_kana;
    SentenceCandidates   sentences;
    bool                 translation_changed = false;
    bool                 sentence_changed    = false;

    auto commit_phrase(const Phrase* phrase) -> void;
    auto commit_all_phrases() -> void;
    auto make_branch_sentence() -> void;
    auto merge_branch_sentences() -> bool;
    auto shrink_sentences(bool reserve_one = false) -> void;
    auto delete_surrounding_text() -> bool;
    auto calc_phrase_in_cursor(Phrase** phrase, size_t* cursor_in_phrase = nullptr) const -> void;
    auto move_cursor_back() -> void;
    auto append_kana(const std::string& kana) -> void;
    auto translate_phrases(const Phrases& source, bool best_only) -> Sentences;
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
