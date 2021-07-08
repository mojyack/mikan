#pragma once
#include <fcitx/inputcontextproperty.h>

#include "phrase.hpp"
#include "share.hpp"

namespace mikan {
class MikanEngine;
class MikanState final : public fcitx::InputContextProperty {
  private:
    fcitx::InputContext& context;
    MikanEngine&         engine;
    Share&               share;
    Phrases*             phrases = nullptr;
    size_t               cursor;              // in bytes
    std::vector<size_t>  romaji_table_indexs; // used in handle_romaji().
    std::string          to_kana;
    bool                 translation_changed = false;
    SentenceCandidates   sentences;
    bool                 sentence_changed = false;

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
    auto reinterpret_sentence() -> void;
    auto build_preedit_text() const -> fcitx::Text;
    auto apply_candidates() -> void;
    auto auto_commit() -> void;
    auto reset() -> void;

    auto handle_romaji(const fcitx::KeyEvent& event) -> bool;
    auto handle_delete(const fcitx::KeyEvent& event) -> bool;
    auto handle_reinterpret_phrases(const fcitx::KeyEvent& event) -> bool;
    auto handle_candidates(const fcitx::KeyEvent& event) -> bool;
    auto handle_commit_phrases(const fcitx::KeyEvent& event) -> bool;
    auto handle_move_cursor_phrase(const fcitx::KeyEvent& event) -> bool;
    auto handle_split_phrase(const fcitx::KeyEvent& event) -> bool;
    auto handle_merge_phrase(const fcitx::KeyEvent& event) -> bool;
    auto handle_move_separator(const fcitx::KeyEvent& event) -> bool;
    auto handle_convert_katakana(const fcitx::KeyEvent& event) -> bool;

    const static std::vector<std::function<bool(MikanState*, const fcitx::KeyEvent&)>> handlers;

  public:
    auto handle_key_event(fcitx::KeyEvent& event) -> void;
    auto handle_activate() -> void;
    auto handle_deactivate() -> void;

    MikanState(fcitx::InputContext& context, MikanEngine& engine, Share& share);
};

} // namespace mikan
