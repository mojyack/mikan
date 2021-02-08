#pragma once
#include <fcitx/event.h>
#include <fcitx/inputcontext.h>
#include <fcitx/inputcontextproperty.h>

#include "type.hpp"

class MikanEngine;
class MikanState final : public fcitx::InputContextProperty {
  private:
    fcitx::InputContext& context;
    MikanEngine&         engine;
    Share&               share;

    std::optional<size_t> cursor; // in bytes
    std::vector<Phrase>   phrases;
    std::string           to_kana;
    bool                  move_back_on_input = false;

    bool        is_candidate_list_active(Phrase* phrase = nullptr);
    void        commit_phrase(const Phrase* phrase);
    bool        delete_surrounding_text();
    void        calc_phrase_in_cursor(Phrase** phrase, size_t* cursor_in_phrase = nullptr) const;
    void        move_cursor_back();
    void        append_kana(const std::string& kana);
    void        translate_phrases(std::vector<Phrase>* target = nullptr);
    fcitx::Text build_preedit_text() const;
    void        apply_candidates();
    void        auto_commit();

    bool handle_romaji(const fcitx::KeyEvent& event);
    bool handle_delete(const fcitx::KeyEvent& event);
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
