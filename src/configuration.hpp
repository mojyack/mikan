#pragma once
#include <fcitx-utils/key.h>
#include <fcitx-utils/keysym.h>
#include <fcitx/event.h>

namespace mikan {
enum class Actions : size_t {
    Backspace = 0,
    ReinterpretNext,
    ReinterpretPrev,
    CandidateNext,
    CandidatePrev,
    CandidatePageNext,
    CandidatePagePrev,
    Commit,
    WordNext,
    WordPrev,
    SplitWordLeft,
    SplitWordRight,
    MergeWordsLeft,
    MergeWordsRight,
    TakeFromLeft,
    TakeFromRight,
    GiveToLeft,
    GiveToRight,
    ConvertKatakana,
    EnterCommandMode,
    ExitCommandMode,
    ActionsLimit,
};

struct KeyConfigKey {
    fcitx::Key key;
    bool       on_press   = true;
    bool       on_release = false;

    KeyConfigKey(fcitx::KeySym sym, fcitx::KeyStates state = fcitx::KeyStates()) : key(fcitx::Key(sym, state)) {}
};

struct KeyConfig {
    std::vector<std::vector<KeyConfigKey>> keys;

    auto operator[](Actions action) -> std::vector<KeyConfigKey>& {
        return keys[static_cast<size_t>(action)];
    }

    template <size_t len>
    auto match(const Actions (&&actions)[len], const fcitx::KeyEvent& event) const -> std::optional<Actions> {
        for(const auto a : actions) {
            if(match(a, event)) {
                return a;
            }
        }
        return {};
    }

    auto match(const Actions action, const fcitx::KeyEvent& event) const -> bool {
        const auto& key = event.key();
        for(const auto& k : keys[static_cast<size_t>(action)]) {
            if(key.check(k.key) && ((k.on_press && !event.isRelease()) || (k.on_release && event.isRelease()))) {
                return true;
            }
        }
        return false;
    }
};
} // namespace mikan
