#pragma once
#include <fcitx-utils/key.h>
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
    PhraseNext,
    PhrasePrev,
    SplitPhraseLeft,
    SplitPhraseRight,
    MergePhraseLeft,
    MergePhraseRight,
    MoveSeparatorLeft,
    MoveSeparatorRight,
    ConvertKatakana,
    ActionsLimit,
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
