#pragma once
#include <fcitx-utils/key.h>
#include <fcitx/event.h>

namespace mikan {
enum class Actions : size_t {
    BACKSPACE = 0,
    REINTERPRET_NEXT,
    REINTERPRET_PREV,
    CANDIDATE_NEXT,
    CANDIDATE_PREV,
    CANDIDATE_PAGE_NEXT,
    CANDIDATE_PAGE_PREV,
    COMMIT,
    PHRASE_NEXT,
    PHRASE_PREV,
    SPLIT_PHRASE_LEFT,
    SPLIT_PHRASE_RIGHT,
    MERGE_PHRASE_LEFT,
    MERGE_PHRASE_RIGHT,
    MOVE_SEPARATOR_LEFT,
    MOVE_SEPARATOR_RIGHT,
    CONVERT_KATAKANA,
    ACTIONS_LIMIT,
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
