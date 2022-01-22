#pragma once
#include <fcitx-utils/key.h>
#include <fcitx/event.h>

namespace mikan {
enum class Actions : size_t {
    Backspace = 0,
    Reinterpret_next,
    Reinterpret_prev,
    Candidate_next,
    Candidate_prev,
    Candidate_page_next,
    Candidate_page_prev,
    Commit,
    Phrase_next,
    Phrase_prev,
    Split_phrase_left,
    Split_phrase_right,
    Merge_phrase_left,
    Merge_phrase_right,
    Move_separator_left,
    Move_separator_right,
    Convert_katakana,
    Actions_limit,
};

struct KeyConfigKey {
    fcitx::Key key;
    bool       on_press   = true;
    bool       on_release = false;

    KeyConfigKey(fcitx::KeySym sym, fcitx::KeyStates state = fcitx::KeyStates());
};

struct KeyConfig {
    std::vector<std::vector<KeyConfigKey>> keys;

    auto operator[](Actions action) -> std::vector<KeyConfigKey>&;
    auto match(const std::vector<Actions>& actions, const fcitx::KeyEvent& event) const -> bool;
    auto match(Actions action, const fcitx::KeyEvent& event) const -> bool;
};
} // namespace mikan
