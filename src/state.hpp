#pragma once
#include <fcitx/event.h>
#include <fcitx/inputcontext.h>
#include <fcitx/inputcontextproperty.h>

#include "phrase.hpp"
#include "share.hpp"
#include "type.hpp"

class MikanEngine;
class MikanState final : public fcitx::InputContextProperty {
  private:
    fcitx::InputContext& context;
    MikanEngine&         engine;
    Share&               share;

    Phrases*    phrases = nullptr;
    size_t      cursor; // in bytes
    std::string to_kana;
    bool        translation_changed = false;

    SentenceCandidates sentences;
    bool               sentence_changed = false;

    void        commit_phrase(const Phrase* phrase);
    void        commit_all_phrases();
    void        make_branch_sentence();
    bool        merge_branch_sentences();
    void        shrink_sentences(bool reserve_one = false);
    bool        delete_surrounding_text();
    void        calc_phrase_in_cursor(Phrase** phrase, size_t* cursor_in_phrase = nullptr) const;
    void        move_cursor_back();
    void        append_kana(const std::string& kana);
    Sentences   translate_phrases(const Phrases& source, bool best_only);
    void        reinterpret_sentence();
    fcitx::Text build_preedit_text() const;
    void        apply_candidates();
    void        auto_commit();
    void        reset();

    bool handle_romaji(const fcitx::KeyEvent& event);
    bool handle_delete(const fcitx::KeyEvent& event);
    bool handle_reinterpret_phrases(const fcitx::KeyEvent& event);
    bool handle_candidates(const fcitx::KeyEvent& event);
    bool handle_commit_phrases(const fcitx::KeyEvent& event);
    bool handle_move_cursor_phrase(const fcitx::KeyEvent& event);
    bool handle_split_phrase(const fcitx::KeyEvent& event);
    bool handle_merge_phrase(const fcitx::KeyEvent& event);
    bool handle_move_separator(const fcitx::KeyEvent& event);
    bool handle_convert_katakana(const fcitx::KeyEvent& event);

    const static std::vector<std::function<bool(MikanState*, const fcitx::KeyEvent&)>> handlers;

  public:
    void handle_key_event(fcitx::KeyEvent& event);
    void handle_activate();
    void handle_deactivate();
    MikanState(fcitx::InputContext& context, MikanEngine& engine, Share& share);
};
