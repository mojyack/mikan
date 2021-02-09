#pragma once
#include "fcitx-utils/keysym.h"
#include "fcitx/event.h"
#include <cstddef>
#include <optional>

#include <fcitx-utils/key.h>

enum class Actions : size_t {
    BACKSPACE = 0,
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
    KeyConfigKey(fcitx::KeySym sym, fcitx::KeyStates state = fcitx::KeyStates()) {
        key = fcitx::Key(sym, state);
    }
};

struct KeyConfig {
    std::vector<std::vector<KeyConfigKey>> keys;
    std::vector<KeyConfigKey>&             operator[](Actions action) {
        return keys[static_cast<size_t>(action)];
    }
    bool match(const std::vector<Actions>& actions, const fcitx::KeyEvent& event) {
        for(auto a : actions) {
            if(match(a, event)) {
                return true;
            }
        }
        return false;
    }
    bool match(Actions action, const fcitx::KeyEvent& event) {
        const fcitx::Key& key = event.key();
        for(const auto& k : keys[static_cast<size_t>(action)]) {
            if(key.check(k.key) && ((k.on_press && !event.isRelease()) || (k.on_release && event.isRelease()))) {
                return true;
            }
        }
        return false;
    }
};
